import os
import sys
import argparse
import subprocess

"""
Used to create gcode without writing the whole command line and with the correct context

Usually:
./pcb2gcode --noconfigfile --metric=true --metricoutput=true --mirror-absolute=false
--nog64=false --optimise=true --tile-x=1 --tile-y=1 --vectorial=true --zero-start=true
--extra-passes=2 --laser-feed=300 --offset=0.1500 --zwork=20.0 --custom_milling_start_gcode="M106 S255" --custom_milling_stop_gcode="M107" --back=bottom.TXT --output-dir=.

You can use https://nraynaud.github.io/webgcode/ to check the output file
"""

def get_base_args():
    """
        Create a list of common arguments for pcb2gcode
    """
    base = list()
    if os.path.isfile("./pcb2gcode"):
        base.append("./pcb2gcode")
    elif os.path.isfile("../pcb2gcode"):
        base.append("../pcb2gcode")
    else:
        print "ERROR unable to find pcb2gcode in . or .."
        sys.exit(1)
    base.append("--noconfigfile")           # without config file
    base.append("--metric=true")            # with metric standard
    base.append("--metricoutput=true")      # also metric in output
    base.append("--nog64=true")             # not used in this context
    base.append("--optimise=false")         # better result when optimise is NOT set oO
    base.append("--tile-x=1")               # multiply the number of board in the x axis
    base.append("--tile-y=1")               # multiply the number of board in the y axis
    base.append("--mirror-absolute=false")  # positive position
    base.append("--vectorial=true")         # better result with it
    base.append("--dpi=500")                # default is 1000 but I export mys projects in 500 DPI
    base.append("--mill-speed=0")           # just to avoid error
    return base


def run_pcb2gcode(args, verbose):
    """
        Run a subprocess with the correct arguments
    """
    if verbose:
        print "DEBUG: %s" % " ".join(args)
    subprocess.check_call(args)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="pcb2gcode arguments helper when working with the laser.")
    parser.add_argument("--verbose", "-v", dest="verbose", action="store_true", help="display the called command line")
    parser.add_argument("--extra-passes", "-n", dest="extra_passes", type=int, default=0, help="number of isolation pass")
    parser.add_argument("--diam", "-d", default=0.250, type=float, help="size of the laser point (mm)")
    parser.add_argument("--x_offset", "-x", default=0.0, type=float, help="x position of the pcb origin (mm)")
    parser.add_argument("--y_offset", "-y", default=0.0, type=float, help="y position of the pcb origin (mm)")
    parser.add_argument("--zwork", "-z", default=10.0, type=float, help="z position of the laser while engraving (mm)")
    parser.add_argument("--feed", "-s", default=100, type=float, help="engraving feed speed (mm/s)")
    parser.add_argument("--start", default="\"G04 P1000\nM106 S255\nG04 P250\"", type=str, help="custom_milling_start_gcode")
    parser.add_argument("--stop", default="\"G04 P1500\nM107\nG04 P250\"", type=str, help="custom_milling_stop_gcode")
    parser.add_argument("--out", "-o", default=".", type=str, help="output directory for the output files")
    parser.add_argument("--back", "-b", type=str, help="path for the gerber file")
    parser.add_argument("--front", "-f", type=str, help="path for the gerber file")
    args = parser.parse_args()

    assert args.back or args.front, "input file is not specified"

    cmd_args = get_base_args()
    cmd_args.append("--custom_milling_start_gcode=%s" % args.start)
    cmd_args.append("--custom_milling_stop_gcode=%s" % args.stop)
    cmd_args.append("--extra-passes=%s" % args.extra_passes)
    cmd_args.append("--x-offset=%s" % args.x_offset)
    cmd_args.append("--y-offset=%s" % args.y_offset)
    cmd_args.append("--zwork=%s" % args.zwork)
    cmd_args.append("--zsafe=%s" % (args.zwork + 0.1))
    cmd_args.append("--zchange=%s" % (args.zwork + 0.1))
    cmd_args.append("--mill-feed=%s" % args.feed)
    cmd_args.append("--offset=%s" % (args.diam/2))
    cmd_args.append("--milldrill-diameter=%s" % args.diam)
    cmd_args.append("--output-dir=%s" % args.out)
    if args.back:
        cmd_args.append("--back=%s" % args.back)
    if args.front:
        cmd_args.append("--front=%s" % args.front)

    run_pcb2gcode(cmd_args, args.verbose)
