def control():
    d = {
        "control": {
            "startyear": 2020,
            "startmonth": 1,
            "duration": 130,  # One reactor cycle takes around 40 days.
            "dt": 86400,  # duration of a time step in seconds, here: 1 day
            "simhandle": "gprreactor and VarRecipeSource tutorial file",
        }
    }
    return d
