import json
from argparse import ArgumentParser, ArgumentDefaultsHelpFormatter
from subprocess import CalledProcessError, run, PIPE, STDOUT

import archetypes
import commodity
import control
import facility
import recipe
import region


def main():
    args = parse_args()

    with open(args.input_fname, "w", encoding="utf-8") as f:
        json.dump(simulation(), f, indent=4)

    if not args.run:
        return

    print("Starting Cyclus simulation...")
    cmd = f"cyclus -v11 -i {args.input_fname} -o {args.output_fname}".split()
    completed_process = run(
        cmd, stdout=PIPE, stderr=STDOUT, encoding="utf-8", check=False
    )
    try:
        completed_process.check_returncode()
    except CalledProcessError as e:
        print(f"Cyclus simulation failed with code {completed_process.returncode}")
        raise e
    else:
        print("Cyclus simulation completed successfully.")
    finally:
        with open(args.log_fname, "w", encoding="utf-8") as f:
            f.writelines(completed_process.stdout)


def parse_args():
    parser = ArgumentParser(
        formatter_class=ArgumentDefaultsHelpFormatter,
        description="Generate a Cyclus input file.",
    )
    parser.add_argument(
        "--run",
        action="store_true",
        help="If set, starts a Cyclus simulation as subprocess.",
    )
    parser.add_argument(
        "-i",
        "--input-fname",
        default="input_file.json",
        type=str,
        help="Name of input file",
    )
    parser.add_argument(
        "-o",
        "--output-fname",
        default="cyclus.sqlite",
        type=str,
        help="Name of Cyclus output file",
    )
    parser.add_argument(
        "--log-fname",
        default="simulation.log",
        type=str,
        help="Name of Cyclus log file",
    )

    return parser.parse_args()


def simulation():
    arch = archetypes.archetypes()
    commod = commodity.commodity()
    ctrl = control.control()
    fac = facility.facility()
    recipes = recipe.recipe()
    reg = region.region()

    return {"simulation": {**arch, **commod, **ctrl, **fac, **recipes, **reg}}


if __name__ == "__main__":
    main()
