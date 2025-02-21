#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Define the kernel function used to calculate the predictions.

This module is strongly based on Antonio Figueroa's work, who kindly
provided the original code. For more information, please see
https://doi.org/10.1016/j.anucene.2020.108085
or https://arxiv.org/abs/2006.12921
or https://github.com/FigueroaAC/GPs-for-SpentFuel
"""

__all__ = ["kernel_function"]

import numpy as np
from scipy.spatial.distance import cdist


def kernel_function(X1, X2, type_, *params, gradient=False):
    """
    All the kernels have a bit of noise added in order to prevent the covariance
    matrix becoming singular. The amount of noise to be added is also a
    parameter to reconstruct
    """

    def dif(array1, array2):
        output = [array1[i] - array2[i] for i in range(len(array1))]
        output = np.array(output).ravel()
        return sum(output)

    if type_ == "SQE":
        sqdist = (
            np.sum(X1**2, 1).reshape(-1, 1) + np.sum(X2**2, 1) - 2 * np.dot(X1, X2.T)
        )
        kernel = (params[0][0] ** 2) * np.exp(-0.5 * sqdist / params[0][1] ** 2) + (
            (params[0][-1] ** 2) * np.eye(X1.shape[0])
        )
        if gradient:
            gradients = [
                2 * params[0][0] * kernel,
                np.multiply(sqdist / (params[0][1] ** 3), kernel),
                2 * params[0][-1] * np.eye(X1.shape[0]),
            ]
            return kernel, gradients
        return kernel

    if type_ == "ASQE":
        lambda_ = np.eye(len(X2[0]))
        length_scales = 1 / params[0][1:-1]
        np.fill_diagonal(lambda_, length_scales)

        X1 = np.dot(X1, lambda_)
        X2 = np.dot(X2, lambda_)
        sqdist = cdist(X1, X2, metric="sqeuclidean").T
        kernel = (params[0][0] ** 2) * np.exp(-0.5 * sqdist) + (
            (params[0][-1] ** 2) * np.eye(X1.shape[0])
        )
        if gradient:
            g = [
                cdist(
                    np.expand_dims(X1[:, i], -1),
                    np.expand_dims(X2[:, i], -1),
                    metric="sqeuclidean",
                )
                / (params[0][i + 1])
                for i in range(X1.shape[1])
            ]
            gradients = [np.multiply(g[i], kernel) for i in range(X1.shape[1])]
            gradients = (
                [2 * params[0][0] * kernel]
                + gradients
                + [2 * params[0][-1] * np.eye(X1.shape[0])]
            )

            return kernel, gradients
        return kernel

    if type_ == "LAP":
        dist = np.sqrt(
            np.sum(X1**2, 1).reshape(-1, 1) + np.sum(X2**2, 1) - 2 * np.dot(X1, X2.T)
        )
        kernel = (params[0][0] ** 2) * np.exp(-0.5 * dist / params[0][1]) + (
            (params[0][-1] ** 2) * np.eye(X1.shape[0])
        )
        if gradient:
            gradients = [
                2 * params[0][0] * kernel,
                np.multiply(dist / (params[0][1] ** 2), kernel),
                2 * params[0][-1] * np.eye(X1.shape[0]),
            ]
            return kernel, gradients
        return kernel

    if type_ == "ALAP":
        dist = np.sqrt(
            cdist(X1 / params[0][1:-1], X2 / params[0][1:-1], metric="euclidean").T
        )
        kernel = (params[0][0] ** 2) * np.exp(-0.5 * dist) + (
            (params[0][-1] ** 2) * np.eye(X1.shape[0])
        )
        if gradient:
            g = [
                cdist(
                    np.expand_dims(X1[:, i] / params[0][i + 1], -1),
                    np.expand_dims(X2[:, i] / params[0][i + 1], -1),
                    metric="euclidean",
                )
                / (params[0][i + 1])
                for i in range(X1.shape[1])
            ]
            gradients = [np.multiply(g[i], kernel) for i in range(X1.shape[1])]
            gradients = (
                [2 * params[0][0] * kernel]
                + gradients
                + [2 * params[0][-1] * np.eye(X1.shape[0])]
            )

            return kernel, gradients
        return kernel

    if type_ == "Linear":
        # This is a version of Linear modified to include lengthscales for each
        # Input Variable
        # ============================================================================
        #         kernel = a*X.Y+b
        # ============================================================================
        lambda_ = np.eye(len(params[0][2:-1]))
        length_scales = 1 / params[0][2:-1]
        np.fill_diagonal(lambda_, length_scales)
        X1 = np.dot(X1, lambda_)
        X2 = np.dot(X2, lambda_)
        kernel = (
            (params[0][0] * np.dot(X1, X2.T).T)
            + params[0][1]
            + ((params[0][-1] ** 2) * np.eye(X1.shape[0]))
        )

        if gradient:
            gradients = (
                [np.dot(X1, X2.T).T]
                + [np.eye(kernel.shape[0])]
                + [
                    2
                    * params[0][0]
                    * np.dot(
                        np.expand_dims(X1[:, i], -1), np.expand_dims(X2[:, i], -1).T
                    ).T
                    / params[0][i]
                    for i in range(X1.shape[1])
                ]
            )
            gradients = gradients + [2 * params[0][-1] * np.eye(X1.shape[0])]
            return kernel, gradients
        return kernel

    if type_ == "Poly":
        # ============================================================================
        #         kernel = (a*(X.Y)+b)**c
        # ============================================================================
        lambda_ = np.eye(len(params[0][3:-1]))
        length_scales = 1 / params[0][3:-1]
        np.fill_diagonal(lambda_, length_scales)
        X1 = np.dot(X1, lambda_)
        X2 = np.dot(X2, lambda_)
        kernel = ((params[0][0] * np.dot(X1, X2.T).T) + params[0][1]) ** params[0][
            2
        ] + ((params[0][-1] ** 2) * np.eye(X1.shape[0]))

        if gradient:
            db = params[0][2] * ((params[0][0] * np.dot(X1, X2.T)) + params[0][1]) ** (
                params[0][2] - 1
            )
            da = np.multiply(np.dot(X1, X2.T), db)
            dc = np.multiply(
                kernel, np.log((params[0][0] * np.dot(X1, X2.T)) + params[0][1])
            )
            gradients = [da, db, dc] + [
                2
                * params[0][0]
                * np.multiply(
                    np.dot(np.expand_dims(X1[:, i], -1), np.expand_dims(X2[:, i], -1).T)
                    / params[0][i],
                    db,
                )
                for i in range(X1.shape[1])
            ]
            gradients = gradients + [2 * params[0][-1] * np.eye(X1.shape[0])]
            return kernel, gradients
        return kernel

    if type_ == "Anova":
        # This isn't working, probably some parameters for lengthscales are needed for
        # each dimension.
        kernel = (
            np.exp(
                (-params[0][0] * cdist(X1, X2, metric="sqeuclidean")) ** params[0][1]
            )
            + np.exp(
                (-params[0][0] * cdist(X1**2, X2**2, metric="sqeuclidean"))
                ** params[0][1]
            )
            + np.exp(
                (-params[0][0] * cdist(X1**3, X2**3, metric="sqeuclidean"))
                ** params[0][1]
            )
            + ((params[0][-1] ** 2) * np.eye(X1.shape[0]))
        )  # Not working
        if gradient:
            dd = params[0][1] * (
                np.exp(
                    (-params[0][0] * cdist(X1, X2, metric="sqeuclidean"))
                    ** (params[0][1] - 1)
                )
                + np.exp(
                    (-params[0][0] * cdist(X1**2, X2**2, metric="sqeuclidean"))
                    ** (params[0][1] - 1)
                )
                + np.exp(
                    (-params[0][0] * cdist(X1**3, X2**3, metric="sqeuclidean"))
                    ** (params[0][1] - 1)
                )
            )
            gradients = [
                -np.multiply(cdist(X1, X2, metric="sqeuclidean"), dd),
                -np.multiply(cdist(X1**2, X2**2, metric="sqeuclidean"), dd),
                -np.multiply(cdist(X1**3, X2**3, metric="sqeuclidean"), dd),
                dd,
                2 * params[0][-1] * np.eye(X1.shape[0]),
            ]
            return kernel, gradients
        return kernel

    if type_ == "Sigmoid":
        # This is not a positive semidefinite kernelernel. Some tweaking has to be done
        # to make this work. Probably along the lines of KdotK.T
        kernel = np.tanh(params[0][0] * np.dot(X1, X2.T) + params[0][1]) + (
            (params[0][-1] ** 2) * np.eye(X1.shape[0])
        )
        if gradient:
            sech2 = 1 / (np.cosh(params[0][0] * np.dot(X1, X2.T) + params[0][1]) ** 2)
            gradients = [
                np.multiply(np.dot(X1, X2.T), sech2),
                sech2,
                2 * params[0][-1] * np.eye(X1.shape[0]),
            ]
            return kernel, gradients
        # This isn't working, probably some parameters for lengthscales are needed for
        # each dimension.
        return kernel

    if type_ == "RQ":
        lambda_ = np.eye(len(X1[0]))
        length_scales = 1 / params[0][1:-1]
        np.fill_diagonal(lambda_, length_scales)
        X1 = np.dot(X1, lambda_)
        X2 = np.dot(X2, lambda_)
        sqdist = cdist(X1, X2, metric="sqeuclidean").T
        kernel = (
            1
            - (sqdist / (sqdist + (params[0][0]) ** 2))
            + ((params[0][-1] ** 2) * np.eye(X1.shape[0]))
        )

        if gradient:
            denom = 1 / ((sqdist + (params[0][0]) ** 2)) ** 2
            g = [
                cdist(
                    np.expand_dims(X1[:, i], -1),
                    np.expand_dims(X2[:, i], -1),
                    metric="sqeuclidean",
                )
                / (params[0][i + 1])
                for i in range(X1.shape[1])
            ]
            gradients = [
                -2 * denom * g[i] * (params[0][0]) ** 2 for i in range(X1.shape[1])
            ]
            gradients = (
                [2 * params[0][0] * sqdist * denom]
                + gradients
                + [2 * params[0][-1] * np.eye(X1.shape[0])]
            )
            return kernel, gradients

        return kernel

    if type_ == "SRQ":
        lambda_ = np.eye(len(X1[0]))
        length_scales = 1 / params[0][:-1]
        np.fill_diagonal(lambda_, length_scales)
        X1 = np.dot(X1, lambda_)
        X2 = np.dot(X2, lambda_)
        sqdist = cdist(X1, X2, metric="sqeuclidean").T
        return (1 / (1 + (sqdist / params[0][-1]))) ** params[0][-1]

    if type_ == "MultiQuad":
        # This Kernel is not positive semidefinite so a lot of tweaking would
        # have to be done to make this work. Maybe take some product of KdotK.T
        lambda_ = np.eye(len(X1[0]))
        length_scales = 1 / params[0][1:-1]
        np.fill_diagonal(lambda_, length_scales)
        X1 = np.dot(X1, lambda_)
        X2 = np.dot(X2, lambda_)
        sqdist = cdist(X1, X2, metric="sqeuclidean").T
        kernel = np.sqrt(sqdist + (params[0][0] ** 2)) + (
            (params[0][-1] ** 2) * np.eye(X1.shape[0])
        )
        if gradient:
            g = [
                cdist(
                    np.expand_dims(X1[:, i], -1),
                    np.expand_dims(X2[:, i], -1),
                    metric="sqeuclidean",
                )
                / (params[0][i + 1])
                for i in range(X1.shape[1])
            ]
            gradients = [np.multiply(g[i], 1 / kernel) for i in range(X1.shape[1])]
            gradients = (
                [params[0][0] / kernel]
                + gradients
                + [2 * params[0][-1] * np.eye(X1.shape[0])]
            )
            return kernel, gradients

        return kernel

    if type_ == "InvMultiQuad":
        lambda_ = np.eye(len(X1[0]))
        length_scales = 1 / params[0][1:-1]
        np.fill_diagonal(lambda_, length_scales)
        X1 = np.dot(X1, lambda_)
        X2 = np.dot(X2, lambda_)
        sqdist = np.sqrt(cdist(X1, X2, metric="sqeuclidean").T + (params[0][0] ** 2))
        kernel = 1 / sqdist + ((params[0][-1] ** 2) * np.eye(X1.shape[0]))
        if gradient:
            g = [
                cdist(
                    np.expand_dims(X1[:, i], -1),
                    np.expand_dims(X2[:, i], -1),
                    metric="sqeuclidean",
                )
                / (params[0][i + 1])
                for i in range(X1.shape[1])
            ]
            gradients = [-np.multiply(g[i], kernel**3) for i in range(X1.shape[1])]
            gradients = (
                [-params[0][0] * (kernel**3)]
                + gradients
                + [2 * params[0][-1] * np.eye(X1.shape[0])]
            )

            return kernel, gradients
        return kernel
    if type_ == "Wave":
        dist = cdist(X1, X2, metric="euclidean")
        kernel = ((params[0][0] / dist) * np.sin(dist / params[0][0])) + (
            (params[0][-1] ** 2) * np.eye(X1.shape[0])
        )
        if gradient:
            arg = dist / params[0][0]
            gradients = (1 / dist) * (
                np.sin(arg) - (np.cos(arg) / (params[0][0]) ** 2)
            ) + [2 * params[0][-1] * np.eye(X1.shape[0])]
            return kernel, gradients
        return kernel

    if type_ == "Power":
        kernel = cdist(
            X1 / params[0][1:-1], X2 / params[0][1:-1], metric="euclidean"
        ) ** params[0][0] + ((params[0][-1] ** 2) * np.eye(X1.shape[0]))
        if gradient:
            dd = params[0][0] * cdist(
                X1 / params[0][1:-1], X2 / params[0][1:-1], metric="euclidean"
            ) ** (params[0][0] - 1)
            g = [
                cdist(
                    np.expand_dims(X1[:, i] / params[0][i + 1], -1),
                    np.expand_dims(X2[:, i] / params[0][i + 1], -1),
                    metric="sqeuclidean",
                )
                / (params[0][i + 1])
                for i in range(X1.shape[1])
            ]
            gradients = [np.multiply(g[i], dd) for i in range(X1.shape[1])]
            gradients = [dd] + gradients + [2 * params[0][-1] * np.eye(X1.shape[0])]
            return kernel, gradients
        return kernel

    if type_ == "Log":
        # This is a non positive definite Kernel
        dist = cdist(X1, X2, metric="euclidean").T
        kernel = -np.log((dist ** params[0][0]) + 1) + (
            (params[0][-1] ** 2) * np.eye(X1.shape[0])
        )
        if gradient:
            arg = dist ** params[0][0]
            gradients = arg * np.log(dist) / (arg + 1) + [
                2 * params[0][-1] * np.eye(X1.shape[0])
            ]
            return kernel, gradients
        return kernel

    if type_ == "Cauchy":
        # This works very nice and fast
        sqdist = cdist(X1, X2, metric="sqeuclidean").T
        kernel = 1 / (1 + (sqdist / (params[0][0] ** 2))) + (
            (params[0][-1] ** 2) * np.eye(X1.shape[0])
        )
        if gradient:
            gradients = [sqdist / (((params[0][0] ** 2) + sqdist) ** 2)] + [
                2 * params[0][-1] * np.eye(X1.shape[0])
            ]
            return kernel, gradients
        return kernel

    if type_ == "Tstudent":
        # This isn't working at the moment
        lambda_ = np.eye(len(X1[0]))
        length_scales = 1 / params[0][1:]
        np.fill_diagonal(lambda_, length_scales)
        X1 = np.dot(X1, lambda_)
        X2 = np.dot(X2, lambda_)
        sqdist = cdist(X1, X2, metric="euclidean").T
        kernel = 1 / (1 + (sqdist ** params[0][0])) + (
            (params[0][-1] ** 2) * np.eye(X1.shape[0])
        )
        if gradient:
            da = -(sqdist ** params[0][0]) * np.log(sqdist) * (kernel**2)
            arg = (sqdist ** (params[0][0] - 2)) * (kernel**2)
            gradients = (
                [da]
                + [
                    params[0][0]
                    * np.multiply(
                        cdist(
                            np.expand_dims(X1[:, i], -1),
                            np.expand_dims(X2[:, i], -1),
                            metric="sqeuclidean",
                        )
                        / (params[0][i + 1]),
                        arg,
                    )
                    for i in range(X1.shape[1])
                ]
                + [2 * params[0][-1] * np.eye(X1.shape[0])]
            )
            return kernel, gradients
        return kernel
