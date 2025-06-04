#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Calculate the composition of spent fuel based on a set of trained GPRs.

This module is part of the misoenrichment module for Cyclus, see
https://github.com/maxschalz/miso_enrichment/ .

The module uses Benjamin Jung's Gaussian Process regression work, see
https://github.com/jung-benjamin/gp-surfer or https://doi.org/10.5281/zenodo.15576628.
The authors also would like to thank Antonio Figueroa, whose GPR code was used
previously.
"""

__author__ = "Nuclear Verification and Disarmament Group, RWTH Aachen University"
__copyright__ = (
    "Copyright 2020-2025, Nuclear Verification and Disarmament Group, "
    "RWTH Aachen University"
)
__credits__ = ["Antonio Figueroa", "Benjamin Jung", "Max Schalz"]
__license__ = "BSD-3-Clause"
__version__ = "2.0"
__maintainer__ = "Max Schalz"

from .predict_posterior import *
