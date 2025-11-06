#!/usr/bin/env python3

"""
This script is ported from the JavaScript version at https://bertdobbelaere.github.io/sorting_networks.html.

The input file is a sorting network in text format, like:
[(0,1),(2,3)]
[(0,2),(1,3)]
[(1,2)]
"""

import argparse
import os
import subprocess
from typing import List, Tuple

VERT_GRID = 32
DOT_RADIUS = 6
FINE_SPACING = 16
GROUP_SPACING = 40
SVG_MAX_WIDTH = 800


def connect_inputs_svg(
    pair: Tuple[int, int], xpos: float, vert_grid: float, dot_radius: float
) -> str:
    """
    Generate SVG elements for connecting two inputs with a comparator.

    Args:
        pair: Tuple of (input1, input2) indices
        xpos: X position for the comparator
        vert_grid: Vertical grid spacing
        dot_radius: Radius of connection dots

    Returns:
        SVG string for the comparator
    """
    y1 = vert_grid / 2.0 + vert_grid * pair[0]
    y2 = vert_grid / 2.0 + vert_grid * pair[1]

    return (
        f'<circle cx="{xpos}" cy="{y1}" r="{dot_radius}" fill="black"/>\n'
        f'<circle cx="{xpos}" cy="{y2}" r="{dot_radius}" fill="black"/>\n'
        f'<line x1="{xpos}" y1="{y1}" x2="{xpos}" y2="{y2}" '
        f'style="stroke:rgb(0,0,0);stroke-width:1"/>\n'
    )


def center_of_gravity(plst: List[Tuple[int, int]]) -> float:
    """
    Calculate the center of gravity for a list of pairs.

    Args:
        plst: List of (input1, input2) pairs

    Returns:
        Center of gravity value
    """
    n = 2.0 * len(plst)
    s = sum(p[0] + p[1] for p in plst)
    return s / n


def span(plst: List[Tuple[int, int]]) -> int:
    """
    Calculate the span (max - min) of all values in the pair list.

    Args:
        plst: List of (input1, input2) pairs

    Returns:
        Span value
    """
    if not plst:
        return 0

    flat_list = []
    for p in plst:
        flat_list.extend([p[0], p[1]])

    return max(flat_list) - min(flat_list)


def run_visual_optimizer(sublayers: List[List[Tuple[int, int]]]) -> None:
    """
    Optimize the visual layout of sublayers by sorting them.

    Args:
        sublayers: List of sublayers, each containing pairs
    """
    # Sort by span first, then by center of gravity
    sublayers.sort(key=lambda x: (span(x), center_of_gravity(x)))


def parse_sorting_network(text: str) -> Tuple[List[List[Tuple[int, int]]], int]:
    """
    Parse sorting network text into layers and determine number of inputs.

    Args:
        text: Sorting network text in format like "[(0,8),(1,7)]\n[(0,1),(2,5)]"

    Returns:
        Tuple of (layers, num_inputs)
    """
    lines = text.strip().split("\n")
    layers = []
    n_inputs = 0

    for line in lines:
        line = line.strip()
        if line == "":
            continue
        if line.startswith("#"):
            continue
        if line == "END":
            break

        # Remove brackets and split by pairs
        line = line.replace("[", "").replace("]", "")
        pairs = line.split("),(")

        els = []
        for pair in pairs:
            # Clean up the pair string
            pair = pair.replace("(", "").replace(")", "")
            if "," in pair:
                parts = pair.split(",")
                if len(parts) >= 2:
                    try:
                        l = int(parts[0])
                        h = int(parts[1])
                        if l >= n_inputs:
                            n_inputs = l + 1
                        if h >= n_inputs:
                            n_inputs = h + 1
                        els.append((l, h))
                    except ValueError:
                        continue

        if els:
            layers.append(els)

    return layers, n_inputs


def generate_svg(network_text: str) -> str:
    """
    Generate SVG visualization from sorting network text.

    Args:
        network_text: Sorting network in text format

    Returns:
        SVG content as string
    """
    layers, n_inputs = parse_sorting_network(network_text)

    if not layers or n_inputs == 0:
        return "<svg></svg>"

    # Process layers into detailed sublayers
    detailed_layers = []
    n_sublayers = 0

    for layer in layers:
        # Sort layer by span and center position
        layer_sorted = sorted(
            layer,
            key=lambda p: (
                (p[1] - p[0]),  # span
                abs(p[0] + p[1] - n_inputs),  # distance from center
            ),
        )

        sublayers = []
        section_use = []

        for pair in layer_sorted:
            i, j = pair[0], pair[1]
            idx = 0

            # Find first sublayer that doesn't conflict
            while idx < len(sublayers):
                fit = True
                for s in range(i, j):
                    if s in section_use[idx]:
                        fit = False
                        break
                if fit:
                    break
                idx += 1

            # Create new sublayer if needed
            if idx >= len(sublayers):
                sublayers.append([])
                section_use.append({})
                n_sublayers += 1

            sublayers[idx].append(pair)
            for s in range(i, j):
                section_use[idx][s] = 1

        run_visual_optimizer(sublayers)
        detailed_layers.append(sublayers)

    # Calculate dimensions and scaling
    n_width = len(layers) * GROUP_SPACING + n_sublayers * FINE_SPACING
    scale = 1.0
    if n_width > SVG_MAX_WIDTH:
        scale = SVG_MAX_WIDTH / n_width

    s_vert_grid = scale * VERT_GRID
    s_dot_radius = scale * DOT_RADIUS
    s_fine_spacing = scale * FINE_SPACING
    s_group_spacing = scale * GROUP_SPACING
    width = scale * n_width

    # Generate SVG
    svg_content = f'<svg style="background-color: #FFF;" width="{width}" height="{s_vert_grid * n_inputs}">\n'

    # Draw horizontal lines for each input
    for n in range(n_inputs):
        y = s_vert_grid / 2.0 + n * s_vert_grid
        svg_content += f'<line x1="0" y1="{y}" x2="{width}" y2="{y}" style="stroke:rgb(0,0,0);stroke-width:1"/>\n'

    # Draw comparators
    xpos = s_group_spacing / 2.0
    for dl in detailed_layers:
        for sl in dl:
            for pair in sl:
                svg_content += connect_inputs_svg(pair, xpos, s_vert_grid, s_dot_radius)
            xpos += s_fine_spacing
        xpos += s_group_spacing

    svg_content += "</svg>\n"
    return svg_content


def main():
    """Main function to generate SVG from sorting network file."""
    parser = argparse.ArgumentParser(
        description="Generate SVG visualization from sorting network file"
    )
    parser.add_argument(
        "filename", help="Input sorting network file (e.g., abc.network)"
    )
    args = parser.parse_args()

    # Read sorting network from file
    with open(args.filename, "r") as f:
        network_text = f.read()

    # Generate output filename
    base_name = os.path.splitext(args.filename)[0]
    svg_filename = f"{base_name}.svg"
    png_filename = f"{base_name}.png"
    pdf_filename = f"{base_name}.pdf"

    # Generate SVG
    svg_content = generate_svg(network_text)

    # Save SVG to file
    with open(svg_filename, "w") as f:
        f.write(svg_content)

    # Convert to PNG and PDF
    print(
        f"Converting SVG to {png_filename} and {pdf_filename} using ImageMagick convert command"
    )
    subprocess.call(["convert", svg_filename, png_filename])
    subprocess.call(["convert", svg_filename, pdf_filename])


if __name__ == "__main__":
    main()
