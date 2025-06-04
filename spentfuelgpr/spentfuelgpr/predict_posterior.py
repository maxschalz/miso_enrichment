#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Interface used by Cyclus to calculate the spent fuel composition.

See __init__.py for information on the 'gaussianprocesses' module.
"""

import json
from pathlib import Path

import gaussianprocesses as nvd_gp


def predict(uid_fname=""):
    """Predict the spent fuel composition."""
    in_fname = f"gpr_reactor_input_params{uid_fname}.json"
    with open(in_fname, "r", encoding="utf-8") as f:
        data = json.load(f)

    input_params = [data[k] for k in data["gp_input_params_and_order"].split()]

    kernel_filepaths_fname = Path(data["path_to_kernel_filepaths"]).absolute()
    gpr_collection = nvd_gp.models.ModelCollection.from_json(
        kernel_filepaths_fname, base=kernel_filepaths_fname.parent
    )
    y_predict = gpr_collection.calculate_predictions([input_params])

    out_fname = f"gpr_reactor_spent_fuel_composition{uid_fname}.json"
    with open(out_fname, "w", encoding="utf-8") as f:
        json.dump(y_predict, f, indent=2)
