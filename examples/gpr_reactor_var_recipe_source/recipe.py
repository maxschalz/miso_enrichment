def recipe():
    d = {
        "recipe": [
            {
                "name": "NaturalURecipe",
                "basis": "mass",
                "nuclide": [
                    {"id": "U234", "comp": 5.5e-3},
                    {"id": "U235", "comp": 0.711},
                    {"id": "U238", "comp": 100 - 0.711 - 5.5e-3},
                ],
            }
        ]
    }
    return d
