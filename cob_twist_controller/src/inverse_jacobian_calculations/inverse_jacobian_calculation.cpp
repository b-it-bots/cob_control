/*!
 *****************************************************************
 * \file
 *
 * \note
 *   Copyright (c) 2015 \n
 *   Fraunhofer Institute for Manufacturing Engineering
 *   and Automation (IPA) \n\n
 *
 *****************************************************************
 *
 * \note
 *   Project name: care-o-bot
 * \note
 *   ROS stack name: cob_control
 * \note
 *   ROS package name: cob_twist_controller
 *
 * \author
 *   Author: Marco Bezzon, email: Marco.Bezzon@ipa.fraunhofer.de
 *
 * \date Date of creation: May, 2015
 *
 * \brief
 *   Implementation pseudoinverse calculation classes.
 *
 ****************************************************************/
#include <ros/ros.h>
#include <Eigen/Core>
#include <Eigen/SVD>

#include <cob_twist_controller/inverse_jacobian_calculations/inverse_jacobian_calculation.h>

/**
 * Calculates the pseudoinverse of the Jacobian by using SVD technique.
 * This allows to get information about singular values and evaluate them.
 */
Eigen::MatrixXd PInvBySVD::calculate(const Eigen::MatrixXd& jacobian) const
{
    Eigen::JacobiSVD<Eigen::MatrixXd> svd(jacobian, Eigen::ComputeThinU | Eigen::ComputeThinV);
    double eps_truncation = DIV0_SAFE;  // prevent division by 0.0
    Eigen::VectorXd singularValues = svd.singularValues();
    Eigen::VectorXd singularValuesInv = Eigen::VectorXd::Zero(singularValues.rows());

    // small change to ref: here quadratic damping due to Control of Redundant Robot Manipulators : R.V. Patel, 2005, Springer [Page 13-14]
    for (uint32_t i = 0; i < singularValues.rows(); ++i)
    {
        double denominator = singularValues(i) * singularValues(i);
        // singularValuesInv(i) = (denominator < eps_truncation) ? 0.0 : singularValues(i) / denominator;
        singularValuesInv(i) = (singularValues(i) < eps_truncation) ? 0.0 : singularValues(i) / denominator;
    }

    Eigen::MatrixXd pseudoInverseJacobian = svd.matrixV() * singularValuesInv.asDiagonal() * svd.matrixU().transpose();

    return pseudoInverseJacobian;
}

/**
 * Calculates the pseudoinverse of the Jacobian by using SVD technique.
 * This allows to get information about singular values and evaluate them.
 */
Eigen::MatrixXd PInvBySVD::calculate(const TwistControllerParams& params,
                                     boost::shared_ptr<DampingBase> db,
                                     const Eigen::MatrixXd& jacobian) const
{
    Eigen::JacobiSVD<Eigen::MatrixXd> svd(jacobian, Eigen::ComputeThinU | Eigen::ComputeThinV);
    double eps_truncation = params.eps_truncation;
    Eigen::VectorXd singularValues = svd.singularValues();
    Eigen::VectorXd singularValuesInv = Eigen::VectorXd::Zero(singularValues.rows());
    double lambda = db->getDampingFactor(singularValues, jacobian);
    uint32_t i = 0;

    if (params.numerical_filtering)
    {
        // Formula 20 Singularity-robust Task-priority Redundandancy Resolution
        // Sum part
        for (; i < singularValues.rows()-1; ++i)
        {
            // pow(beta, 2) << pow(lambda, 2)
            singularValuesInv(i) = singularValues(i) / (pow(singularValues(i), 2) + pow(params.beta, 2));
        }
        // Formula 20 - additional part
        singularValuesInv(i) = singularValues(i) / (pow(singularValues(i), 2) + pow(params.beta, 2) + pow(lambda, 2));
    }
    else
    {
        // small change to ref: here quadratic damping due to Control of Redundant Robot Manipulators : R.V. Patel, 2005, Springer [Page 13-14]
        for (; i < singularValues.rows(); ++i)
        {
            double denominator = (singularValues(i) * singularValues(i) + pow(lambda, 2) );
            // singularValuesInv(i) = (denominator < eps_truncation) ? 0.0 : singularValues(i) / denominator;
            singularValuesInv(i) = (singularValues(i) < eps_truncation) ? 0.0 : singularValues(i) / denominator;
        }

        //// Formula from Advanced Robotics : Redundancy and Optimization : Nakamura, Yoshihiko, 1991, Addison-Wesley Pub. Co [Page 258-260]
        // for(uint32_t i = 0; i < singularValues.rows(); ++i)
        // {
        //       // damping is disabled due to damping factor lower than a const. limit
        //       singularValues(i) = (singularValues(i) < eps_truncation) ? 0.0 : 1.0 / singularValues(i);
        // }
    }

    Eigen::MatrixXd pseudoInverseJacobian = svd.matrixV() * singularValuesInv.asDiagonal() * svd.matrixU().transpose();

    return pseudoInverseJacobian;
}

/**
 * Calculates the pseudoinverse by means of left/right pseudo inverse respectively.
 */
Eigen::MatrixXd PInvDirect::calculate(const Eigen::MatrixXd& jacobian) const
{
    Eigen::MatrixXd result;
    Eigen::MatrixXd j_t = jacobian.transpose();
    uint32_t jac_rows = jacobian.rows();
    uint32_t jac_cols = jacobian.cols();

    if (jac_cols >= jac_rows)
    {
        result = j_t * (jacobian * j_t).inverse();
    }
    else
    {
        result = (j_t * jacobian).inverse() * j_t;
    }

    return result;
}

/**
 * Calculates the pseudoinverse by means of left/right pseudo inverse respectively.
 */
Eigen::MatrixXd PInvDirect::calculate(const TwistControllerParams& params,
                                      boost::shared_ptr<DampingBase> db,
                                      const Eigen::MatrixXd& jacobian) const
{
    Eigen::MatrixXd result;
    Eigen::MatrixXd j_t = jacobian.transpose();
    uint32_t jac_rows = jacobian.rows();
    uint32_t jac_cols = jacobian.cols();
    if (params.damping_method == LEAST_SINGULAR_VALUE)
    {
        ROS_ERROR("PInvDirect does not support SVD. Use PInvBySVD class instead!");
    }

    double lambda = db->getDampingFactor(Eigen::VectorXd::Zero(1, 1), jacobian);
    if (jac_cols >= jac_rows)
    {
        Eigen::MatrixXd ident = Eigen::MatrixXd::Identity(jac_rows, jac_rows);
        Eigen::MatrixXd toBeInv = jacobian * j_t + lambda * lambda * ident;
        result = j_t * toBeInv.inverse();
    }
    else
    {
        Eigen::MatrixXd ident = Eigen::MatrixXd::Identity(jac_cols, jac_cols);
        Eigen::MatrixXd toBeInv = j_t * jacobian + lambda * lambda * ident;
        result = toBeInv.inverse() * j_t;
    }

    return result;
}
