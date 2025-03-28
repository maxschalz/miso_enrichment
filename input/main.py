import json

import archetypes
import commodity
import control
import facility
import recipe
import region


def simulation():
    arch = archetypes.archetypes()
    commod = commodity.commodity()
    ctrl = control.control()
    fac = facility.facility()
    recipes = recipe.recipe()
    reg = region.region()

    return {"simulation": {**arch, **commod, **ctrl, **fac, **recipes, **reg}}


if __name__ == "__main__":
    with open("input_file.json", "w") as f:
        json.dump(simulation(), f, indent=4)
