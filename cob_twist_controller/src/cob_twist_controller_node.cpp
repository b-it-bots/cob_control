/*!
 *****************************************************************
 * \file
 *
 * \note
 *   Copyright (c) 2014 \n
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
 *   Author: Felix Messmer, email: Felix.Messmer@ipa.fraunhofer.de
 *
 * \date Date of creation: April, 2014
 *
 * \brief
 *   This package provides a generic Twist controller for the Care-O-bot
 *
 ****************************************************************/
#include <ros/ros.h>
#include <cob_twist_controller/cob_twist_controller.h>

int main(int argc, char **argv)
{
    ros::init(argc, argv, "cob_twist_controller_node");
    CobTwistController* cob_twist_controller = new CobTwistController();

    if (!cob_twist_controller->initialize())
    {
        ROS_ERROR("Failed to initialize TwistController");
        return -1;
    }

    ros::spin();
    return 0;
}
