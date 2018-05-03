/*
 * This file is part of pcb2gcode.
 *
 * Copyright (C) 2009, 2010 Patrick Birnzain <pbirnzain@users.sourceforge.net>
 * Copyright (C) 2010 Bernhard Kubicek <kubicek@gmx.at>
 * Copyright (C) 2013 Erik Schuster <erik@muenchen-ist-toll.de>
 * Copyright (C) 2014-2017 Nicola Corna <nicola@corna.info>
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

#include "options.hpp"
#include "config.h"

#include <fstream>
#include <list>
#include <boost/exception/all.hpp>
#include <boost/algorithm/string.hpp>
#include "units.hpp"

#include <iostream>
using std::cerr;
using std::endl;

/******************************************************************************/
/*
 */
/******************************************************************************/
options&
options::instance()
{
    static options singleton;
    return singleton;
}

/******************************************************************************/
/*
 */
/******************************************************************************/
void options::parse(int argc, char** argv)
{
    // guessing causes problems when one option is the start of another
    // (--drill, --drill-diameter); see bug 3089930
    int style = po::command_line_style::default_style
                & ~po::command_line_style::allow_guessing;

    po::options_description generic;
    generic.add(instance().cli_options).add(instance().cfg_options);

    try
    {
        po::store(po::parse_command_line(argc, argv, generic, style),
                  instance().vm);
    }
    catch (std::logic_error& e)
    {
        cerr << "Error: You've supplied an invalid parameter.\n"
             << "Note that spindle speeds are integers!\n" << "Details: "
             << e.what() << endl;
        exit(ERR_UNKNOWNPARAMETER);
    }

    po::notify(instance().vm);

    if( !instance().vm["noconfigfile"].as<bool>() )
        parse_files();

    /*
     * this needs to be an extra step, as --basename modifies the default
     * values of the --...-output parameters
     */
    string basename = "";

    if (instance().vm.count("basename"))
    {
        basename = instance().vm["basename"].as<string>() + "_";
    }

    string front_output = "--front-output=" + basename + "front.ngc";
    string back_output = "--back-output=" + basename + "back.ngc";
    string outline_output = "--outline-output=" + basename + "outline.ngc";
    string drill_output = "--drill-output=" + basename + "drill.ngc";

    const char *fake_basename_command_line[] = { "", front_output.c_str(),
                                                 back_output.c_str(),
                                                 outline_output.c_str(),
                                                 drill_output.c_str()
                                               };

    po::store(
        po::parse_command_line(5, (char**) fake_basename_command_line,
                               generic, style),
        instance().vm);


    if (instance().vm.count("tolerance"))
    {
        if (instance().vm.count("g64"))
        {
            cerr << "You can't specify both tolerance and g64!\n";
            exit(ERR_BOTHTOLERANCEG64);
        }
    }
    else
    {
        const double cfactor = instance().vm["metric"].as<bool>() ? 25.4 : 1;
        double tolerance;

        if (instance().vm.count("g64"))
        {
            tolerance = instance().vm["g64"].as<double>();
        }
        else
        {
            if (instance().vm["vectorial"].as<bool>())
                tolerance = 0.0004 * cfactor;
            else
                tolerance = 2.0 / instance().vm["dpi"].as<int>() * cfactor;
        }

        string tolerance_str = "--tolerance=" + to_string(tolerance);

        const char *fake_tolerance_command_line[] = { "",
                                            tolerance_str.c_str() };

        po::store(po::parse_command_line(2,
                        (char**) fake_tolerance_command_line,
                        generic, style), instance().vm);
    }

    po::notify(instance().vm);
}

/******************************************************************************/
/*
 */
/******************************************************************************/
string options::help()
{
    std::stringstream msg;
    msg << PACKAGE_STRING << "\n\n";
    msg << instance().cli_options << instance().cfg_options;
    return msg.str();
}

/******************************************************************************/
/*
 */
/******************************************************************************/
void options::parse_files()
{

    std::string file("millproject");

    try
    {
        std::ifstream stream;

        try
        {
            stream.open(file.c_str());
            po::store(po::parse_config_file(stream, instance().cfg_options),
                      instance().vm);
        }
        catch (std::exception& e)
        {
            cerr << "Error parsing configuration file \"" << file << "\": "
                 << e.what() << endl;
        }

        stream.close();
    }
    catch (std::exception& e)
    {
        cerr << "Error reading configuration file \"" << file << "\": "
             << e.what() << endl;
    }

    po::notify(instance().vm);
}

/******************************************************************************/
/*
 */
/******************************************************************************/
options::options()
         : cli_options("command line only options"), cfg_options("generic options (CLI and config files)") {

   cli_options.add_options()
       ("noconfigfile", po::value<bool>()->default_value(false)->implicit_value(true), "ignore any configuration file")
       ("help,?", "produce help message")
       ("version", "show the current software version");
   cfg_options.add_options()
       ("front", po::value<string>(),"front side RS274-X .gbr")
       ("back", po::value<string>(), "back side RS274-X .gbr")
       ("outline", po::value<string>(), "pcb outline polygon RS274-X .gbr")
       ("drill", po::value<string>(), "Excellon drill file")
       ("svg", po::value<string>(), "[DEPRECATED] use --vectorial, SVGs will be generated automatically; this option has no effect")
       ("zwork", po::value<Length>(), "milling depth in inches (Z-coordinate while engraving)")
       ("zsafe", po::value<Length>(), "safety height (Z-coordinate during rapid moves)")
       ("offset", po::value<Length>()->default_value(Length(0)), "distance between the PCB traces and the end mill path in inches; usually half the isolation width")
       ("voronoi", po::value<bool>()->default_value(false)->implicit_value(true), "generate voronoi regions (requires --vectorial)")
       ("pre-milling-gcode", po::value<std::vector<string>>()->default_value(std::vector<string>{}, ""), "custom gcode inserted before the start of milling each trace (used to activate pump or fan or laser connected to fan)")
       ("post-milling-gcode", po::value<std::vector<string>>()->default_value(std::vector<string>{}, ""), "custom gcode inserted after the end of milling each trace (used to deactivate pump or fan or laser connected to fan)")
       ("spinup-time", po::value<Time>()->default_value(parse_unit<Time>("1 ms")), "time required to the spindle to reach the correct speed")
       ("spindown-time", po::value<Time>(), "time required to the spindle to return to 0 rpm")
       ("mill-feed", po::value<Velocity>(), "feed while isolating in [i/m] or [mm/m]")
       ("mill-vertfeed", po::value<Velocity>(), "vertical feed while isolating in [i/m] or [mm/m]")
       ("x-offset", po::value<double>()->default_value(0.0), "distance between the PCB origin and the machine origin in the X axe")
       ("y-offset", po::value<double>()->default_value(0.0), "distance between the PCB origin and the machine origin in the Y axe")
       ("mill-speed", po::value<Frequency>(), "spindle rpm when milling")
       ("milldrill", po::value<bool>()->default_value(false)->implicit_value(true), "drill using the mill head")
       ("milldrill-diameter", po::value<Length>(), "diameter of the end mill used for drilling with --milldrill")
       ("nog81", po::value<bool>()->default_value(false)->implicit_value(true), "replace G81 with G0+G1")
       ("nog91-1", po::value<bool>()->default_value(false)->implicit_value(true), "do not explicitly set G91.1 in drill headers")
       ("extra-passes", po::value<int>()->default_value(0), "specify the the number of extra isolation passes, increasing the isolation width half the tool diameter with each pass")
       ("fill-outline", po::value<bool>()->default_value(true)->implicit_value(true), "accept a contour instead of a polygon as outline (enabled by default)")
       ("outline-width", po::value<Length>()->default_value(parse_unit<Length>("1.5mm")), "width of the outline, used only when vectorial is disabled")
       ("cutter-diameter", po::value<Length>(), "diameter of the end mill used for cutting out the PCB")
       ("zcut", po::value<Length>(), "PCB cutting depth in inches")
       ("cut-feed", po::value<Velocity>(), "PCB cutting feed in [i/m] or [mm/m]")
       ("cut-vertfeed", po::value<Velocity>(), "PCB vertical cutting feed in [i/m] or [mm/m]")
       ("cut-speed", po::value<Frequency>(), "spindle rpm when cutting")
       ("cut-infeed", po::value<Length>(), "maximum cutting depth; PCB may be cut in multiple passes")
       ("cut-front", po::value<bool>()->implicit_value(true), "[DEPRECATED, use cut-side instead] cut from front side. ")
       ("cut-side", po::value<BoardSide::BoardSide>()->default_value(BoardSide::AUTO), "cut side; valid choices are front, back or auto (default)")
       ("zdrill", po::value<Length>(), "drill depth")
       ("zchange", po::value<Length>(), "tool changing height")
       ("zchange-absolute", po::value<bool>()->default_value(false)->implicit_value(true), "use zchange as a machine coordinates height (G53)")
       ("drill-feed", po::value<Velocity>(), "drill feed in [i/m] or [mm/m]")
       ("drill-speed", po::value<Frequency>(), "spindle rpm when drilling")
       ("drill-front", po::value<bool>()->implicit_value(true), "[DEPRECATED, use drill-side instead] drill through the front side of board")
       ("drill-side", po::value<BoardSide::BoardSide>()->default_value(BoardSide::AUTO), "drill side; valid choices are front, back or auto (default)")
       ("onedrill", po::value<bool>()->default_value(false)->implicit_value(true), "use only one drill bit size")
       ("metric", po::value<bool>()->default_value(false)->implicit_value(true), "use metric units for parameters. does not affect gcode output")
       ("metricoutput", po::value<bool>()->default_value(false)->implicit_value(true), "use metric units for output")
       ("optimise", po::value<bool>()->default_value(true)->implicit_value(true), "Reduce output file size by up to 40% while accepting a little loss of precision (enabled by default).")
       ("eulerian-paths", po::value<bool>()->default_value(true)->implicit_value(true), "Don't mill the same path twice if milling loops overlap.  This can save up to 50% of milling time.  Enabled by default.")
       ("bridges", po::value<Length>()->default_value(Length(0)), "add bridges with the given width to the outline cut")
       ("bridgesnum", po::value<unsigned int>()->default_value(2), "specify how many bridges should be created")
       ("zbridges", po::value<Length>(), "bridges height (Z-coordinates while engraving bridges, default to zsafe) ")
       ("tile-x", po::value<int>()->default_value(1), "number of tiling columns. Default value is 1")
       ("tile-y", po::value<int>()->default_value(1), "number of tiling rows. Default value is 1")
       ("al-front", po::value<bool>()->default_value(false)->implicit_value(true), "enable the z autoleveller for the front layer")
       ("al-back", po::value<bool>()->default_value(false)->implicit_value(true),
            "enable the z autoleveller for the back layer")
       ("software", po::value<Software::Software>(), "choose the destination software (useful only with the autoleveller). Supported programs are linuxcnc, mach3, mach4 and custom")
       ("al-x", po::value<Length>(), "width of the x probes")
       ("al-y", po::value<Length>(), "width of the y probes")
       ("al-probefeed", po::value<Velocity>(), "speed during the probing")
       ("al-probe-on", po::value<string>()->default_value("(MSG, Attach the probe tool)@M0 ( Temporary machine stop. )"), "execute this commands to enable the probe tool (default is M0)")
       ("al-probe-off", po::value<string>()->default_value("(MSG, Detach the probe tool)@M0 ( Temporary machine stop. )"), "execute this commands to disable the probe tool (default is M0)")
       ("al-probecode", po::value<string>()->default_value("G31"), "custom probe code (default is G31)")
       ("al-probevar", po::value<unsigned int>()->default_value(2002), "number of the variable where the result of the probing is saved (default is 2002)")
       ("al-setzzero", po::value<string>()->default_value("G92 Z0"), "gcode for setting the actual position as zero (default is G92 Z0)")
       ("dpi", po::value<int>()->default_value(1000), "virtual photoplot resolution")
       ("vectorial", po::value<bool>()->default_value(false)->implicit_value(true), "EXPERIMENTAL!! Enable the experimental vectorial core")
       ("zero-start", po::value<bool>()->default_value(false)->implicit_value(true), "set the starting point of the project at (0,0)")
       ("g64", po::value<double>(), "[DEPRECATED, use tolerance instead] maximum deviation from toolpath, overrides internal calculation")
       ("tolerance", po::value<double>(), "maximum toolpath tolerance")
       ("nog64", po::value<bool>()->default_value(false)->implicit_value(true), "do not set an explicit g64")
       ("mirror-absolute", po::value<bool>()->default_value(true)->implicit_value(true), "[DEPRECATED, must always be true] mirror back side along absolute zero instead of board center")
       ("mirror-axis", po::value<Length>()->default_value(Length(0)), "For two-sided boards, the PCB needs to be flipped along the axis x=VALUE")
       ("tsp-2opt", po::value<bool>()->default_value(true)->implicit_value(true), "use TSP 2OPT to find a faster toolpath (but slows down gcode generation)")
       ("output-dir", po::value<string>()->default_value(""), "output directory")
       ("basename", po::value<string>(), "prefix for default output file names")
       ("front-output", po::value<string>()->default_value("front.ngc"), "output file for front layer")
       ("back-output", po::value<string>()->default_value("back.ngc"), "output file for back layer")
       ("outline-output", po::value<string>()->default_value("outline.ngc"), "output file for outline")
       ("drill-output", po::value<string>()->default_value("drill.ngc"), "output file for drilling")
       ("preamble-text", po::value<string>(), "preamble text file, inserted at the very beginning as a comment.")
       ("preamble", po::value<string>(), "gcode preamble file, inserted at the very beginning.")
       ("postamble", po::value<string>(), "gcode postamble file, inserted before M9 and M2.")
       ("no-export", po::value<bool>()->default_value(false)->implicit_value(true), "skip the exporting process");
}

/******************************************************************************/
/*
 */
/******************************************************************************/
static void check_generic_parameters(po::variables_map const& vm)
{
    double unit;      //factor for imperial/metric conversion

    unit = vm["metric"].as<bool>() ? (1. / 25.4) : 1;

    //---------------------------------------------------------------------------
    //Check dpi parameter:

    if (vm["dpi"].as<int>() < 100)
    {
        cerr << "Warning: very low DPI value." << endl;
    }
    else if (vm["dpi"].as<int>() > 10000)
    {
        cerr << "Warning: very high DPI value, processing may take extremely long"
             << endl;
    }

    //---------------------------------------------------------------------------
    //Check spinup(down)-time parameters:

    if (vm["spinup-time"].as<Time>().asMillisecond(1) < 0)
    {
        cerr << "spinup-time can't be negative!\n";
        exit(ERR_NEGATIVESPINUP);
    }

    if (vm.count("spindown-time") && vm["spindown-time"].as<Time>().asMillisecond(1) < 0)
    {
        cerr << "spindown-time can't be negative!\n";
        exit(ERR_NEGATIVESPINDOWN);
    }

    //---------------------------------------------------------------------------
    //Check g64 parameter:

    if (vm.count("g64"))
    {
        cerr << "g64 is deprecated, use tolerance.\n";
    }

    //---------------------------------------------------------------------------
    //Check mirror-absolute parameter:

    if (!vm["mirror-absolute"].as<bool>())
    {
        cerr << "mirror-absolute is deprecated, it must be true.\n";
        exit(ERR_FALSEMIRRORABSOLUTE);
    }

    //---------------------------------------------------------------------------
    //Check tolerance parameter:

    //Upper threshold value of tolerance parameter for warning
    double tolerance_th = vm["metric"].as<bool>() ? 0.2 : 0.008;

    if (vm.count("tolerance"))              //tolerance parameter is given
    {
        if (vm["tolerance"].as<double>() > tolerance_th)
            cerr << "Warning: high tolerance value (allowed deviation from toolpath) given.\n"
                 << endl;

        else if (vm["tolerance"].as<double>() == 0)
            cerr << "Warning: Deviation from commanded toolpath set to 0 (tolerance=0). No smooth milling is most likely!\n"
                 << endl;

        else if (vm["tolerance"].as<double>() < 0)
        {
            cerr << "tolerance can't be negative!\n";
            exit(ERR_NEGATIVETOLERANCE);
        }
    }

    //---------------------------------------------------------------------------
    //Check svg parameter:

    if (vm.count("svg"))
    {
        cerr << "--svg is deprecated and has no effect anymore, use --vectorial to generate SVGs.\n";
    }

    //---------------------------------------------------------------------------
    //Check for available board dimensions:

    if (vm.count("drill")
            && !(vm.count("front") || vm.count("back") || vm.count("outline")))
    {
        cerr << "Warning: Board dimensions unknown. Gcode for drilling will be probably misaligned.\n";
    }

    //---------------------------------------------------------------------------
    //Check for tile parameters

    if (vm["tile-x"].as<int>() < 1)
    {
        cerr << "tile-x can't be negative!\n";
        exit(ERR_NEGATIVETILEX);
    }

    if (vm["tile-y"].as<int>() < 1)
    {
        cerr << "tile-y can't be negative!\n";
        exit(ERR_NEGATIVETILEY);
    }

    //---------------------------------------------------------------------------
    //Check for safety height parameter:

    if (!vm.count("zsafe"))
    {
        cerr << "Error: Safety height not specified.\n";
        exit(ERR_NOZSAFE);
    }

    //---------------------------------------------------------------------------
    //Check for zchange parameter parameter:

    if (!vm.count("zchange"))
    {
        cerr << "Error: Tool changing height not specified.\n";
        exit(ERR_NOZCHANGE);
    }

    //---------------------------------------------------------------------------
    //Check for autoleveller parameters

    if (vm["al-front"].as<bool>() || vm["al-back"].as<bool>())
    {
        if (!vm.count("software"))
        {
            cerr << "Error: unspecified or unsupported software, please specify a supported software (linuxcnc, mach3, mach4 or custom).\n";
            exit(ERR_NOSOFTWARE);
        }

        if (!vm.count("al-x"))
        {
            cerr << "Error: autoleveller probe width x not specified.\n";
            exit(ERR_NOALX);
        }
        else if (vm["al-x"].as<Length>().asInch(unit) <= 0)
        {
            cerr << "Error: al-x < 0!" << endl;
            exit(ERR_NEGATIVEALX);
        }

        if (!vm.count("al-y"))
        {
            cerr << "Error: autoleveller probe width y not specified.\n";
            exit(ERR_NOALY);
        }
        else if (vm["al-y"].as<Length>().asInch(unit) <= 0)
        {
            cerr << "Error: al-y < 0!" << endl;
            exit(ERR_NEGATIVEALY);
        }

        if (!vm.count("al-probefeed"))
        {
            cerr << "Error: autoleveller probe feed rate not specified.\n";
            exit(ERR_NOALPROBEFEED);
        }
        else if (vm["al-probefeed"].as<Velocity>().asInchPerMinute(unit) <= 0)
        {
            cerr << "Error: al-probefeed < 0!" << endl;
            exit(ERR_NEGATIVEPROBEFEED);
        }

    }
}

/******************************************************************************/
/*
 */
/******************************************************************************/
static void check_milling_parameters(po::variables_map const& vm)
{

    double unit;      //factor for imperial/metric conversion

    unit = vm["metric"].as<bool>() ? (1. / 25.4) : 1;

    if (vm.count("front") || vm.count("back"))
    {

        if (!vm.count("zwork"))
        {
            cerr << "Error: --zwork not specified.\n";
            exit(ERR_NOZWORK);
        }
        else if (vm["zwork"].as<Length>().asDouble() > 0)
        {
            cerr << "Warning: Engraving depth (--zwork) is greater than zero!\n";
        }

        if (vm["voronoi"].as<bool>())
        {
            if (!vm["vectorial"].as<bool>())
            {
                cerr << "Error: --voronoi requires --vectorial.\n";
                exit(ERR_VORONOINOVECTORIAL);
            }
        }
        else
        {
            if (!vm.count("offset"))
            {
                cerr << "Error: Engraving --offset not specified.\n";
                exit(ERR_NOOFFSET);
            }
        }

        if (!vm.count("mill-feed"))
        {
            cerr << "Error: Milling feed [i/m or mm/m] not specified.\n";
            exit(ERR_NOMILLFEED);
        }

        if (!vm.count("mill-speed"))
        {
            cerr << "Error: Milling speed [rpm] not specified.\n";
            exit(ERR_NOMILLSPEED);
        }

        // required parameters present. check for validity.
        if (vm["zsafe"].as<Length>().asInch(unit) <= vm["zwork"].as<Length>().asInch(unit))
        {
            cerr << "Error: The safety height --zsafe is lower than the milling "
                 << "height --zwork. Are you sure this is correct?\n";
            exit(ERR_ZSAFELOWERZWORK);
        }

        if (vm["mill-feed"].as<Velocity>().asDouble() <= 0)
        {
            cerr << "Error: Negative or equal to 0 milling feed (--mill-feed).\n";
            exit(ERR_NEGATIVEMILLFEED);
        }

        if (vm.count("mill-vertfeed") && vm["mill-vertfeed"].as<Velocity>().asInchPerMinute(unit) <= 0)
        {
            cerr << "Error: Negative or equal to 0 vertical milling feed (--mill-vertfeed).\n";
            exit(ERR_NEGATIVEMILLVERTFEED);
        }

        if (vm["mill-speed"].as<Frequency>().asDouble() < 0)
        {
            cerr << "Error: --mill-speed < 0.\n";
            exit(ERR_NEGATIVEMILLSPEED);
        }
    }
}

/******************************************************************************/
/*
 */
/******************************************************************************/
static void check_drilling_parameters(po::variables_map const& vm)
{

    double unit;      //factor for imperial/metric conversion

    unit = vm["metric"].as<bool>() ? (1. / 25.4) : 1;

    //only check the parameters if a drill file is given
    if (vm.count("drill"))
    {

        if (!vm.count("zdrill"))
        {
            cerr << "Error: Drilling depth (--zdrill) not specified.\n";
            exit(ERR_NOZDRILL);
        }

        if (vm["zsafe"].as<Length>().asInch(unit) <= vm["zdrill"].as<Length>().asInch(unit))
        {
            cerr << "Error: The safety height --zsafe is lower than the drilling "
                 << "height --zdrill!\n";
            exit(ERR_ZSAFELOWERZDRILL);
        }

        if (!vm.count("zchange"))
        {
            cerr << "Error: Drill bit changing height (--zchange) not specified.\n";
            exit(ERR_NOZCHANGE);
        }
        else if (vm["zchange"].as<Length>().asInch(unit) <= vm["zdrill"].as<Length>().asInch(unit))
        {
            cerr << "Error: The safety height --zsafe is lower than the tool "
                 << "change height --zchange!\n";
            exit(ERR_ZSAFELOWERZCHANGE);
        }

        if (!vm.count("drill-feed"))
        {
            cerr << "Error:: Drilling feed (--drill-feed) not specified.\n";
            exit(ERR_NODRILLFEED);
        }
        else if (vm["drill-feed"].as<Velocity>().asInchPerMinute(unit) <= 0)
        {
            cerr << "Error: The drilling feed --drill-feed is <= 0.\n";
            exit(ERR_NEGATIVEDRILLFEED);
        }

        if (!vm.count("drill-speed"))
        {
            cerr << "Error: Drilling spindle RPM (--drill-speed) not specified.\n";
            exit(ERR_NODRILLSPEED);
        }
        else if (vm["drill-speed"].as<Frequency>().asPerMinute(1) < 0)         //no need to support both directions?
        {
            cerr << "Error: --drill-speed < 0.\n";
            exit(ERR_NEGATIVEDRILLSPEED);
        }

        if (vm.count("drill-front"))
        {
            cerr << "drill-front is deprecated, use drill-side.\n";

            if (!vm["drill-side"].defaulted())
            {
                cerr << "You can't specify both drill-front and drill-side!\n";
                exit(ERR_BOTHDRILLFRONTSIDE);
            }
        }
    }

}

/******************************************************************************/
/*
 */
/******************************************************************************/
static void check_cutting_parameters(po::variables_map const& vm)
{

    double unit;      //factor for imperial/metric conversion

    unit = vm["metric"].as<bool>() ? (1. / 25.4) : 1;

    //only check the parameters if an outline file is given or milldrill is enabled
    if (vm.count("outline") || (vm.count("drill") && vm["milldrill"].as<bool>()))
    {
        if (vm["fill-outline"].as<bool>() && !vm["vectorial"].as<bool>())
        {
            if (!vm.count("outline-width"))
            {
                cerr << "Error: For outline filling, a width (--outline-width) has to be specified.\n";
                exit(ERR_NOOUTLINEWIDTH);
            }
            else
            {
                double outline_width = vm["outline-width"].as<Length>().asInch(unit);
                if (outline_width < 0)
                {
                    cerr << "Error: Specified outline width is less than zero!\n";
                    exit(ERR_NEGATIVEOUTLINEWIDTH);
                }
                else if (outline_width == 0)
                {
                    cerr << "Error. Specified outline width is zero!\n";
                    exit(ERR_ZEROOUTLINEWIDTH);
                }
                else
                {
                    if (outline_width >= 0.4) {
                        cerr << "Warning: You specified an outline-width of "
                             << vm["outline-width"].as<Length>() << "!\n";
                    }
                }
            }
        }

        if (!vm.count("zcut"))
        {
            cerr << "Error: Board cutting depth (--zcut) not specified.\n";
            exit(ERR_NOZCUT);
        }
        else if (vm["zcut"].as<Length>().asInch(unit) > 0)
        {
            cerr << "Error: Cutting depth (--zcut) is greater than zero!\n";
            exit(ERR_NEGATIVEZWORK);
        }

        if (!vm.count("cutter-diameter"))
        {
            cerr << "Error: Cutter diameter not specified.\n";
            exit(ERR_NOCUTTERDIAMETER);
        }

        if (!vm.count("cut-feed"))
        {
            cerr << "Error: Board cutting feed (--cut-feed) not specified.\n";
            exit(ERR_NOCUTFEED);
        }

        if (!vm.count("cut-speed"))
        {
            cerr << "Error: Board cutting spindle RPM (--cut-speed) not specified.\n";
            exit(ERR_NOCUTSPEED);
        }

        if (!vm.count("cut-infeed"))
        {
            cerr << "Error: Board cutting infeed (--cut-infeed) not specified.\n";
            exit(ERR_NOCUTINFEED);
        }

        if (vm["zsafe"].as<Length>().asInch(unit) <= vm["zcut"].as<Length>().asInch(unit))
        {
            cerr << "Error: The safety height --zsafe is lower than the cutting "
                 << "height --zcut!\n";
            exit(ERR_ZSAFELOWERZCUT);
        }

        if (vm["cut-feed"].as<Velocity>().asInchPerMinute(unit) <= 0)
        {
            cerr << "Error: The cutting feed --cut-feed is <= 0.\n";
            exit(ERR_NEGATIVECUTFEED);
        }

        if (vm.count("cut-vertfeed") && vm["cut-vertfeed"].as<Velocity>().asInchPerMinute(unit) <= 0)
        {
            cerr << "Error: The cutting vertical feed --cut-vertfeed is <= 0.\n";
            exit(ERR_NEGATIVECUTVERTFEED);
        }

        if (vm["cut-speed"].as<Frequency>().asPerMinute(1) < 0)        //no need to support both directions?
        {
            cerr << "Error: The cutting spindle speed --cut-speed is lower than 0.\n";
            exit(ERR_NEGATIVESPINDLESPEED);
        }

        if (vm["cut-infeed"].as<Length>().asInch(unit) < 0.001)
        {
            cerr << "Error: The cutting infeed --cut-infeed. seems too low.\n";
            exit(ERR_LOWCUTINFEED);
        }

        if (vm["bridges"].as<Length>().asInch(unit) < 0)
        {
            cerr << "Error: negative bridge value.\n";
            exit(ERR_NEGATIVEBRIDGE);
        }

        if (vm["bridges"].as<Length>().asInch(unit) > 0 && !vm["optimise"].as<bool>() &&
                !vm["vectorial"].as<bool>() )
        {
            cerr << "Error: \"bridges\" requires either \"optimise\" or \"vectorial\".\n";
            exit(ERR_BRIDGENOOPTIMISE);
        }

        if (vm.count("cut-front"))
        {
            cerr << "cut-front is deprecated, use cut-side.\n";

            if (!vm["cut-side"].defaulted())
            {
                cerr << "You can't specify both cut-front and cut-side!\n";
                exit(ERR_BOTHCUTFRONTSIDE);
            }
        }
    }
}

/******************************************************************************/
/*
 */
/******************************************************************************/
void options::check_parameters()
{

    po::variables_map const& vm = instance().vm;

    try
    {
        check_generic_parameters(vm);
        check_milling_parameters(vm);
        check_cutting_parameters(vm);
        check_drilling_parameters(vm);
    }
    catch (std::runtime_error& re)
    {
        cerr << "Error: Invalid parameter. :-(\n";
        exit(ERR_INVALIDPARAMETER);
    }
}
