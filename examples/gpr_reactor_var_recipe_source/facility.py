def facility():
    d = {
        "facility": [
            {
                "name": "NaturalUSource",
                "config": {
                    "VarRecipeSource": {
                        "out_commod": "NaturalU",
                        "var_out_recipe": {
                            "mass_or_atom": "mass",
                            "nuclides": {"item": [
                                {
                                    "nuc_id": 922340000,
                                    "rng_properties": {
                                        "distribution": "uniform",
                                        "parameters": {"val": [5.1e-5, 5.4e-5]}
                                    }
                                },
                                {
                                    "nuc_id": 922350000,
                                    "rng_properties": {
                                        "distribution": "normal",
                                        "parameters": {"val": [7.1e-3, 0.01e-3, 0., 1.]}
                                    }
                                },
                                {
                                    "nuc_id": 922380000,
                                    "rng_properties": {
                                        "distribution": "normalisation",
                                        "parameters": {"val": [0., 0.]}
                                    }
                                }
                            ]}
                        },
                        "throughput_vals": {"val": [1000000]},
                        "throughput_times": {"val": [0]}
                    }
                }
            },
            {
                "name": "SpentFuelSink",
                "config": {"Sink": {"in_commods": {"val": ["SpentFuel"]}}},
            },
            {
                "name": "Reactor",
                "config": {
                    "GprReactor": {
                        "in_commods": {"val": ["NaturalU"]},
                        "out_commods": {"val": ["SpentFuel"]},
                        "in_recipes": {"val": ["NaturalURecipe"]},
                        "mass_or_atom_to_gpr": "atom",
                        "n_assem_core": 1,
                        "n_assem_batch": 1,
                        "assem_size": 10000,
                        "cycle_time": 40,
                        "refuel_time": 1,
                        "power_output": 1300,
                        "normalisation_nuclide": 10010000,
                        "path_to_kernel_filepaths": "trained_kernels/reactor_filepaths.json",
                        "gp_input_params_and_order": "922340000 922350000 cycle_time"
                    }
                },
            },
        ]
    }
    return d
