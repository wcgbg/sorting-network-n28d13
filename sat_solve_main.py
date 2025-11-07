import os
import subprocess
import argparse
import time
import glob
import json
import socket

import multiprocessing as mp


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("cnf_dir", type=str)
    parser.add_argument("--solver", type=str, default="minisat")
    parser.add_argument("-j", "--jobs", type=int)
    parser.add_argument(
        "--limit", type=int, help="The number of problems to solve (default = all)"
    )
    parser.add_argument(
        "--reverse", action="store_true", help="Solve the problems in reverse order"
    )
    parser.add_argument(
        "--keep_going",
        action="store_true",
        help="Continue solving other problems even if a solution is found",
    )
    return parser.parse_args()


def solve_sat_by_kissat(cnf_file):
    executable = "kissat"
    process = subprocess.run([executable, cnf_file], capture_output=True, text=True)
    if process.returncode == 10:
        assert "s SATISFIABLE" in process.stdout
        return True
    elif process.returncode == 20:
        assert "s UNSATISFIABLE" in process.stdout
        return False
    else:
        raise ValueError(f"Unknown return code: {process.returncode}")


def solve_sat_by_minisat(cnf_file):
    executable = os.getenv("MINISAT", "minisat")

    # Determine solution file name (remove .cnf.gz or .cnf extension if present)
    solution_file = cnf_file
    if cnf_file.endswith(".cnf.gz"):
        solution_file = cnf_file[:-7]
    elif cnf_file.endswith(".cnf"):
        solution_file = cnf_file[:-4]
    solution_file = solution_file + ".sol"

    process = subprocess.run(
        [executable, cnf_file, solution_file], capture_output=True, text=True
    )
    if process.returncode == 10:
        assert "\nSATISFIABLE" in process.stdout
        return True
    elif process.returncode == 20:
        assert "\nUNSATISFIABLE" in process.stdout
        return False
    else:
        raise ValueError(f"Unknown return code: {process.returncode}")


def solve_sat_by_cryptominisat(cnf_file):
    executable = os.path.expanduser("~/cryptominisat5-mac-arm64/cryptominisat5")
    process = subprocess.run(
        [executable, "--verb", "0", cnf_file], capture_output=True, text=True
    )
    if process.returncode == 10:
        assert "s SATISFIABLE" in process.stdout
        return True
    elif process.returncode == 20:
        assert "s UNSATISFIABLE" in process.stdout
        return False
    else:
        raise ValueError(f"Unknown return code: {process.returncode}")


def solve_sat_by_painless(cnf_file):
    executable = os.path.expanduser("~/painless/build/release/painless_release")
    # The painless solver doesn't support lines starting with 'c'.
    # Remove all lines starting with 'c'.
    with open(cnf_file, "r") as fin:
        cnf_file_nocmt = cnf_file + ".nocmt"
        with open(cnf_file_nocmt, "w") as fout:
            for line in fin:
                if not line.startswith("c "):
                    fout.write(line)
    # MPI
    #   mpirun -np 8 ~/painless/build/release/painless_release -dist xxx.cnf
    # Multi-threads on one machine.
    process = subprocess.run(
        [executable, "-c=16", "-simple", cnf_file_nocmt], capture_output=True, text=True
    )
    if process.returncode == 10:
        assert "s SATISFIABLE" in process.stdout
        return True
    elif process.returncode == 20:
        assert "s UNSATISFIABLE" in process.stdout
        return False
    else:
        raise ValueError(f"Unknown return code: {process.returncode}")


def solve_sat_by_mallob(cnf_file):
    cwd = os.path.expanduser("~/mallob")
    process = subprocess.run(
        ["build/mallob", "-mono=" + os.path.abspath(cnf_file), "-t=12", "-satsolver=k"],
        capture_output=True,
        text=True,
        cwd=cwd,
    )
    if process.returncode == 10:
        assert "s SATISFIABLE" in process.stdout
        return True
    elif process.returncode == 20:
        assert "s UNSATISFIABLE" in process.stdout
        return False
    else:
        raise ValueError(f"Unknown return code: {process.returncode}")


def solve(args_tuple):
    """Solves a SAT problem, with caching support.

    Args:
        args_tuple: Tuple of (cnf_file, solver_name)

    Returns:
        Tuple of (cnf_file, is_sat, solver_time)
    """
    (cnf_file, solver) = args_tuple
    # Check if this problem was already solved (cached result)
    done_filename = f"{cnf_file}.done"
    if os.path.exists(done_filename):
        with open(done_filename, "r") as f:
            result = json.load(f)
            return cnf_file, result["is_sat"], 0.0

    # Solve the problem
    time0 = time.time()
    if solver == "kissat":
        is_sat = solve_sat_by_kissat(cnf_file)
    elif solver == "minisat":
        is_sat = solve_sat_by_minisat(cnf_file)
    elif solver == "cryptominisat":
        is_sat = solve_sat_by_cryptominisat(cnf_file)
    elif solver == "painless":
        is_sat = solve_sat_by_painless(cnf_file)
    elif solver == "mallob":
        is_sat = solve_sat_by_mallob(cnf_file)
    else:
        raise ValueError(f"Unknown solver: {solver}")
    time1 = time.time()
    solver_time = time1 - time0

    # Write the result to the done file
    with open(done_filename, "w") as f:
        json.dump(
            {
                "is_sat": is_sat,
                "solver_time": solver_time,
                "hostname": socket.gethostname(),
            },
            f,
        )

    return cnf_file, is_sat, solver_time


def main():
    """Main function: solves SAT problems in parallel from a directory.

    Processes all CNF files in the given directory using the specified solver.
    Supports parallel processing, caching, and early termination when a solution is found.
    """
    args = parse_args()
    jobs = args.jobs if args.jobs is not None else mp.cpu_count()
    print(f"Using {jobs} CPU cores for parallel processing")

    # Find all CNF files (including compressed ones)
    cnf_files = glob.glob(os.path.join(args.cnf_dir, "*.cnf"))
    cnf_files += glob.glob(os.path.join(args.cnf_dir, "*.cnf.gz"))
    cnf_files.sort()
    if args.limit:
        cnf_files = cnf_files[: args.limit]
    print(f"Will solve {len(cnf_files)} problems")

    total_is_sat = False
    total_solver_time = 0
    # Process files in parallel
    with mp.Pool(processes=jobs) as pool:
        # Use imap_unordered for better performance (processes results as they complete)
        results = pool.imap_unordered(
            solve,
            [
                (f, args.solver)
                for f in (cnf_files[::-1] if args.reverse else cnf_files)
            ],
        )
        for i, result in enumerate(results):
            cnf_file, is_sat, solver_time = result
            print(
                f"{i}/{len(cnf_files)}. cnf_file: {cnf_file}, is_sat: {is_sat}, solver_time: {solver_time:.1f} seconds.    ",
                end="\r",
                flush=True,
            )
            total_solver_time += solver_time
            if is_sat:
                total_is_sat = True
                # Early termination if a solution is found (unless --keep_going is set)
                if not args.keep_going:
                    print(f"\n!!! SAT !!! cnf_file: {cnf_file}")
                    pool.terminate()
                    pool.join()
                    break
    print()
    print(f"Is SAT: {total_is_sat}")
    print(f"Total solver time (sum of all solves): {total_solver_time:.1f} seconds")


if __name__ == "__main__":
    wall_clock_begin_time = time.time()
    main()
    wall_clock_end_time = time.time()
    print(
        f"Total wall-clock time: {wall_clock_end_time - wall_clock_begin_time:.1f} seconds"
    )
