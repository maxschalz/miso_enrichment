# pylint: disable=invalid-name

"""
Calculate enrichment processes of a uranium mixture composed of more than two isotopes.

This class is a standalone class that does not need to be embedded in the simulation
framework. The file is a condensed version of previous work of RWTH Aachen's NVD group,
with all functions not necessary to MIsoEnrich having been removed.

References
----------
[1] Wood, H. G. Effects of separation processes on minor uranium isotopes in enrichment
    cascades. Science & Global Security 16, 26–36 (2008).

[2] von Halle, E. Multicomponent isotope separation in matched abundance ratio cascades
    composed of stages with large separation factors. in Proceedings of the 1st
    Workshop on Separation Phenomena in Liquids and Gases (Darmstadt, Germany, 1987).

[3] Harvey, R. D. An optimization method for matched abundance-ratio cascades by
    varying the key weight. (University of Tennessee, Knoxville, TN, 2017).
"""

import json
import warnings

import numpy as np
import numpy.typing as npt
from scipy.optimize import minimize

ALLOWED_NUCLIDES = [
    "922320000",
    "922330000",
    "922340000",
    "922350000",
    "922360000",
    "922380000",
]
MSTAR = 350.5
MASS_HEXAFLUORIDE = 6 * 19
ISOTOPES = np.array([232, 233, 234, 235, 236, 238]) + MASS_HEXAFLUORIDE


def calculate_enrichment_from_file(fname, suppress_warnings=False):
    """Read-in specs from a JSON file, calculate the enrichment and write to file."""
    if suppress_warnings:
        warnings.simplefilter("ignore", category=RuntimeWarning)

    m = Multi_isotope.from_json(fname)
    try:
        m.calculate_enrichment()
    except Exception as e:
        m.pprint()
        raise e

    m.results_to_json(fname)


class Multi_isotope:
    """
    Calculate the enrichment of a multicomponent isotope uranium mixture.

    This class allows to calculate the enrichment parameters of a cascade of given
    shape taking into account U-232, U-233, U-234, U-235 and U-238.
    The references used to program this class are found in the module docstring.
    """

    def __init__(
        self,
        feed_composition: dict[str, float],
        product_assay: float,
        tails_assay: float,
        process: str,
        *,
        alpha_235: float = 1.4,
        feed_qty: float = np.inf,
        product_qty: float = np.inf,
        max_swu: float = np.inf,
    ):
        """
        Create a Multi_isotope object.

        Alternatively, it can also be instantiated directly from a JSON file
        """
        # If the values (most notably the number of stages and the
        # enrichment concentrations) are up-to-date, set variable to true.
        # Else, e.g., after changing desired U235 product concentration,
        # set it to False.
        self.uptodate = False

        self.mstar = MSTAR
        self.check_input(feed_qty, product_qty, max_swu)
        self.xf = np.zeros(len(ISOTOPES), dtype=float)
        self.xp = np.zeros(len(ISOTOPES), dtype=float)
        self.xt = np.zeros(len(ISOTOPES), dtype=float)

        self.set_feed_composition(feed_composition)
        self.set_product_enrichment(product_assay)
        self.set_tails_enrichment(tails_assay)

        self.process = ""
        self.set_enrichment_process(process)
        self.alpha, self.alpha_star = 0.0, 0.0
        self.set_alpha(alpha_235)

        # Checks are performed to ensure that only one of both streams is
        # set to infinity.
        self.f = 0
        self.p = 0
        self.t = 0
        self.swu = 0
        self.user_f = feed_qty
        self.user_p = product_qty
        self.user_swu = max_swu

        self.n_e = 100
        self.n_s = 100

        # If the desired product enrichment cannot be reached because the
        # concentration in minor isotopes is too high, then the
        # asymptotically reached maximal enrichment is stored in this
        # variable, else it is set to nan.
        self.maximal_enrichment = float("nan")

    def check_input(
        self,
        feed_qty: float,
        product_qty: float,
        max_swu: float,
    ):
        """Check the arguments passed when instantiating an object

        This function is called upon instantiation of a Multi_isotope
        object. The arguments passed upon instantiation (or at least some
        of them) are passed to check_input, too, and they undergo some
        checks. If these are not passed corresponding errors are raised.
        """
        if max_swu > 1e298 and feed_qty > 1e298 and product_qty > 1e298:
            msg = (
                "'feed', 'product' and 'max_swu' are set to very large values!\n"
                + "At least one variable of these has to be finite!"
            )
            raise ValueError(msg)

        if max_swu <= 0:
            raise ValueError("'max_swu' has to be stricly positive!")

    @classmethod
    def from_json(cls, fname):
        """Generate a Multi_isotope object using input data from a JSON file.

        See the Multi_isotope.__init__ function for information on the allowed keywords
        or miso_enrichment/src/python_enrichment.h for more information."""
        with open(fname, "r", encoding="utf-8") as file_handle:
            input_data = json.load(file_handle)

        return cls(**input_data)

    def results_to_json(self, fname):
        """Store all enrichment results to a JSON file."""
        out_data = {
            "feed_qty": self.f,
            "product_qty": self.p,
            "tails_qty": self.t,
            "swu": self.swu,
            "process": self.process,
            "alpha_235": self.alpha[3],
            "n_enriching": self.n_e,
            "n_stripping": self.n_s,
        }
        for key, composition in (
            ("feed_composition", self.xf),
            ("product_composition", self.xp),
            ("tails_composition", self.xt),
        ):
            out_data[key] = dict(zip(ALLOWED_NUCLIDES, composition))

        with open(fname, "w", encoding="utf-8") as file_handle:
            json.dump(out_data, file_handle)

    def set_feed_composition(self, composition: dict[str, float] | npt.NDArray[float]):
        """Set and normalise the feed composition in atom/mole fractions.

        The feed must contain U235 and U238, else a ValueError is raised.
        """
        if isinstance(composition, np.ndarray):
            allowed_shape = (len(ALLOWED_NUCLIDES),)
            if composition.shape != allowed_shape:
                raise ValueError(
                    "If feed composition is provided as array, then it must have "
                    f"shape {allowed_shape}"
                )
            if any(composition < 0.0) or any(composition >= 1.0):
                raise ValueError(
                    f"Composition {composition} is not valid! At least one "
                    "nuclide with a concentration < 0 or >= 1!"
                )

            self.uptodate = False
            self.xf = composition / composition.sum()
            return

        if any(nucl not in ALLOWED_NUCLIDES for nucl in composition):
            raise KeyError(
                "Invalid nuclide in specified feed composition.\nNuclides in feed "
                f"composition are:\n{list(composition.keys())}\nAllowed nuclides are\n"
                f"{ALLOWED_NUCLIDES}"
            )
        if "922350000" not in composition:
            raise ValueError("Feed U235 content unspecified!")

        for i, nuclide in enumerate(ALLOWED_NUCLIDES):
            if nuclide in composition:
                val = composition[nuclide]
                if val < 0.0 or val >= 1.0:
                    raise ValueError(
                        f"Composition {composition} is not valid! At least one "
                        "nuclide with a concentration < 0 or >= 1!"
                    )
                self.xf[i] = composition[nuclide]

        self.uptodate = False
        self.xf /= self.xf.sum()

    def set_product_enrichment(self, xp: float):
        """Set the desired enrichment level of the U235 product in atom fractions."""
        if xp < 0 or xp > 1.0 or xp <= self.xf[3]:
            msg = (
                "Target product enrichment has to be in the range (0, 1) and it must "
                "be larger than the feed concentration."
            )
            raise ValueError(msg)

        self.user_xp = xp
        self.uptodate = False

    def set_tails_enrichment(self, xt: float):
        """Set the desired enrichment level of the U235 tails in atom fractions."""
        if xt < 0 or xt > 1.0:
            raise ValueError("Target tails enrichment has to be in the range (0, 1)!")
        if xt >= self.xf[3] and any(self.xf != 0.0):
            raise ValueError(
                "Target tails enrichment must be smaller than the feed concentration."
            )

        self.uptodate = False
        self.user_xt = xt

    def set_enrichment_process(self, process: str):
        """Set the enrichment process to be used."""
        possible_processes = ["centrifuge", "diffusion"]
        if process not in possible_processes:
            msg = f"'process' must be one of: {possible_processes}"
            raise ValueError(msg)

        self.uptodate = False
        self.process = process

    def set_alpha(self, alpha_235: float):
        """Calculate the stage separation factors.

        Check the process used and calculate the corresponding stage separation factors
        taking into account the cascade key weight and all of the isotopes. The
        calculations follow [1].
        """
        if self.process == "centrifuge":
            # The factor 1/3 has the unit of 1/atomic mass in order to keep alpha
            # dimensionless.
            self.alpha = (
                1 + (2 * self.mstar - ISOTOPES[3] - ISOTOPES) * (alpha_235 - 1) / 3
            )

        if self.process == "diffusion":
            self.alpha = ((2 * self.mstar - ISOTOPES[3]) / ISOTOPES) ** 0.5

        self.alpha_star = self.alpha / self.alpha[3] ** 0.5

    def value_function(self, x: npt.NDArray[float]) -> float:
        """Calculate the value of a uranium stream with composition `x`.

        This assumes that U-235 is the matched and U-238 the key isotope, based on [3].
        """
        k = (self.alpha - 1) / (self.alpha[3] - 1)
        abundance_ratio_235 = x[3] / x[5]

        if np.any(k == 0.5):
            raise RuntimeError("k=0.5 in value_function() not yet implemented")

        return (x / (2 * k - 1)).sum() * np.log(abundance_ratio_235)

    def get_swu(self) -> float:
        """Calculate the separative work of the enrichment process."""
        if np.inf in {self.p, self.t} and self.f == np.inf:
            return np.inf

        vf = self.value_function(self.xf)
        vp = self.value_function(self.xp)
        vt = self.value_function(self.xt)

        return vp * self.p + vt * self.t - vf * self.f

    def difference_concentration(self) -> float:
        """Calculate the concentration deviations in product and tails from the target.

        Returns
        -------
        Squared sum of relative concentration differences in product and tails, with
        respect to the enrichment levels requested by the user.
        """
        delta_xp = (self.xp[3] - self.user_xp) / self.user_xp
        delta_xt = (self.xt[3] - self.user_xt) / self.user_xt

        return (delta_xp**2 + delta_xt**2) ** 0.5

    def calculate_concentrations(
        self, n_stages: tuple[float, float] = (None, None)
    ) -> float:
        """Calculate the feed, product and tails concentrations, following [2].

        Parameters
        ----------
        n_stages : (float, float), optional
            The number of stages in the enrichment section (first value) and in the
            stripping section (second value).

        Returns
        -------
        Concentrations deviations in product and tails from the target, see
        `Multi_isotope.difference_concentration`.
        """
        n_e = self.n_e if n_stages[0] is None else n_stages[0]
        n_s = self.n_s if n_stages[1] is None else n_stages[1]

        # pylint: disable=invalid-unary-operand-type
        e = 1.0 / self.alpha_star / (1 - self.alpha_star ** (-n_e))
        s = 1.0 / self.alpha_star / (self.alpha_star ** (n_s + 1) - 1)
        e_sum = (e * self.xf / (e + s)).sum()
        s_sum = (s * self.xf / (e + s)).sum()

        self.xp = e * self.xf / ((e + s) * e_sum)
        self.xt = s * self.xf / ((e + s) * s_sum)

        return self.difference_concentration()

    def calculate_flows(self):
        """Calculate the feed, product and tails material flows, following [2]."""
        e = self.alpha_star ** (-1) / (1 - self.alpha_star ** (-self.n_e))
        s = self.alpha_star ** (-1) / (self.alpha_star ** (self.n_s + 1) - 1)
        e_sum = (e * self.xf / (e + s)).sum()
        s_sum = (s * self.xf / (e + s)).sum()

        p = self.user_f * e_sum
        f = self.user_p / e_sum

        if p < self.user_p:
            self.p = p
            self.f = self.user_f
        else:
            self.f = f
            self.p = self.user_p
        self.t = self.f * s_sum

        self.swu = self.get_swu()
        if self.swu > self.user_swu:
            self.swu = self.user_swu

            self.f = self.swu / (
                self.value_function(self.xp) * e_sum
                + self.value_function(self.xt) * s_sum
                - self.value_function(self.xf)
            )
            self.p = self.f * e_sum
            self.t = self.f * s_sum

    def calculate_enrichment(self):
        """Calculate the complete enrichment process."""
        _, _ = self._calculate_staging()

    def _calculate_staging(self, return_OptimizeResult: bool = False):
        """Get the number of stages needed for the enrichment process.

        Calculate the staging by finding the root of
        ```
        sqrt(
            (self.xp[3]-self.user_xp)**2 / self.user_xp**2
          + (self.xt[3]-self.user_xt)**2 / self.user_xt**2
        )
        ```
        using the scipy.optimize.minimize function. The number of stages is not rounded
        to the nearest integer, as this would introduce significant differences in the
        case of enrichment with gas centrifuges.

        Raises
        ------
        RuntimeError if the optimiser does not exit successfully.
        """
        if self.uptodate:
            return self.n_e, self.n_s

        if self.process == "diffusion":
            n_init_enriching = [500, 1000, 5000]
            n_init_stripping = [100, 500, 1000, 5000]
            upper_bound = 7000
        elif self.process == "centrifuge":
            n_init_enriching = [5, 10, 50]
            n_init_stripping = [1, 5, 10, 50]
            upper_bound = 200
        else:
            msg = "'process' must either be 'centrifuge' or 'diffusion'!"
            raise ValueError(msg)

        lower_bound = 0
        bound = (lower_bound, upper_bound)

        # Try different initial conditions, i.e., number of stages to ensure the
        # minimisation converges.
        for n_stripping in n_init_stripping:
            for n_enriching in n_init_enriching:
                result = minimize(
                    self.calculate_concentrations,
                    x0=(n_enriching, n_stripping),
                    bounds=(bound, bound),
                    method="L-BFGS-B",
                    options={"gtol": 1e-15},
                )
                n = result["x"]

                delta = self.difference_concentration()

                self.n_e = n[0]
                self.n_s = n[1]
                self.calculate_concentrations()
                self.calculate_flows()

                if result["success"] and delta < 1e-7:
                    self.uptodate = True
                    if return_OptimizeResult:
                        return result

                    if (
                        "NORM_OF_PROJECTED_GRADIENT" in result["message"]
                        or n[0] > 0.9 * upper_bound
                    ):
                        result = minimize(
                            self.calculate_concentrations,
                            x0=(10 * upper_bound, n_stripping),
                            method="L-BFGS-B",
                            options={"gtol": 1e-15},
                        )
                        n = result["x"]

                        self.calculate_concentrations(n)
                        self.calculate_flows()
                        self.maximal_enrichment = self.xp[3]
                        self.n_e = float("nan")
                        self.n_s = n[1]

                        raise RuntimeError(
                            "Unphysical result:\n"
                            "n_enriching is larger than 0.9 * upper_bound with "
                            f"upper_bound = {upper_bound}. The most probable reason "
                            "is that the concentration of minor isotopes is too high, "
                            "making an U235 product enrichment up to the defined "
                            "level impossible. The maximal (asymptotical) U235 "
                            f"product enrichment is {self.maximal_enrichment:.3f} "
                            "atom fractions. Try lowering the desired U235 enrichment "
                            "below this value (e.g., by 0.5%)."
                        )
                    return n

        raise RuntimeError(f"Optimiser did not exit successfully. Output:\n{result}")

    def enrichment_result_as_dict(self, **kwargs):
        """Return all information on the enrichment process as dict."""
        return {
            "f": self.f,
            "p": self.p,
            "t": self.t,
            "swu": self.swu,
            "xf": self.xf,
            "xp": self.xp,
            "xt": self.xt,
            "alpha_235": self.alpha[3],
            "process": self.process,
            "n_e": self.n_e,
            "n_s": self.n_s,
            **kwargs,
        }

    def pprint(self):
        """Pretty print the input variables and the enrichment results"""
        self.calculate_enrichment()

        str_fmt = "11.3f"
        print("\n--------------------------------------")
        print("Starting calculations with parameters:")
        print(f"  feed               {self.user_f:{str_fmt}}")
        print(f"  product            {self.user_p:{str_fmt}}")
        print(f"  x_p (235)          {self.user_xp:{str_fmt}}")
        print(f"  x_t (235)          {self.user_xt:{str_fmt}}")
        print(f"  process            {self.process:>11}")
        print(f"  maximal SWU        {self.user_swu:{str_fmt}}")
        if self.process == "centrifuge":
            print(f"  alpha_235           {self.alpha[3]:{str_fmt}}")
        print("\nUsed:")
        print(f"  feed               {self.f:{str_fmt}}")
        print(f"  SWU                {self.swu:{str_fmt}}")
        print(f"  enriching stages   {self.n_e:{str_fmt}}")
        print(f"  stripping stages   {self.n_s:{str_fmt}}")
        print("\nProduced:")
        print(f"  product            {self.p:{str_fmt}}")
        print(f"  tails              {self.t:{str_fmt}}")
        print("\nCompositions [%]:")
        print(
            "  U-isotope        232         233         234"
            + "         235         236         238"
        )
        print(
            "  x_f     "
            + (6 * "{:12.4e}").format(
                self.xf[0] * 100,
                self.xf[1] * 100,
                self.xf[2] * 100,
                self.xf[3] * 100,
                self.xf[4] * 100,
                self.xf[5] * 100,
            )
        )
        print(
            "  x_p     "
            + (6 * "{:12.4e}").format(
                self.xp[0] * 100,
                self.xp[1] * 100,
                self.xp[2] * 100,
                self.xp[3] * 100,
                self.xp[4] * 100,
                self.xp[5] * 100,
            )
        )
        print(
            "  x_t     "
            + (6 * "{:12.4e}").format(
                self.xt[0] * 100,
                self.xt[1] * 100,
                self.xt[2] * 100,
                self.xt[3] * 100,
                self.xt[4] * 100,
                self.xt[5] * 100,
            )
        )
        print(
            "\n  alpha   "
            + (6 * "{:12.6f}").format(
                self.alpha[0],
                self.alpha[1],
                self.alpha[2],
                self.alpha[3],
                self.alpha[4],
                self.alpha[5],
            )
        )
