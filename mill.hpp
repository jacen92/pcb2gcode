/*
 * This file is part of pcb2gcode.
 *
 * Copyright (C) 2009, 2010 Patrick Birnzain <pbirnzain@users.sourceforge.net>
 * Copyright (C) 2015 Nicola Corna <nicola@corna.info>
 *
 * pcb2gcode is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * pcb2gcode is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with pcb2gcode.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MILL_H
#define MILL_H

#include <stdint.h>
#include <iostream>
/******************************************************************************/
/*
 */
/******************************************************************************/
class Mill
{
public:
    virtual ~Mill()
    {
    }
    ;

    double feed;
    double vertfeed;
    int speed;
    double zchange;
    double zsafe;
    double zwork;
    double tolerance;
    bool explicit_tolerance;
    bool backside;
    bool mirror_absolute;
    std::string custom_milling_start_gcode;    // when the tool start to mill
    std::string custom_milling_stop_gcode;     // when the tool stop to mill
};

/******************************************************************************/
/*
 */
/******************************************************************************/
class RoutingMill: public Mill
{
public:
    double tool_diameter;
    bool optimise;
};

/******************************************************************************/
/*
 */
/******************************************************************************/
class Isolator: public RoutingMill
{
public:
    int extra_passes;
};

/******************************************************************************/
/*
 */
/******************************************************************************/
class Cutter: public RoutingMill
{
public:
    bool do_steps;
    double stepsize;
    unsigned int bridges_num;
    double bridges_height;
    double bridges_width;
};

/******************************************************************************/
/*
 */
/******************************************************************************/
class Driller: public Mill
{
};

#endif // MILL_H
