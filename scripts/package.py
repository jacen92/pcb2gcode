#!/usr/bin/python

import os
import sys
import json
import shutil
import platform
import argparse
import subprocess

"""
    Used to create a debian package with and without all dependancies
"""

def get_libraries(target):
    """
        Run ldd on the target executable
    """
    libs = dict()
    args = list()
    args.append('ldd')
    args.append(target)
    sp = subprocess.check_output(args)
    sp = sp.replace("\t", "")
    lines = sp.split(os.linesep)
    for line in lines:
        index = line.split(" => ")[0]
        if index.find("libc.") != -1:
            pass
        elif index.find("ld-linux") != -1:
            pass
        else:
            libs[index] = line.split(" => ")[-1].split(" ")[0]
    return libs


def get_arch():
    host_arch = platform.machine()
    if host_arch in ['x86_64', 'amd64']:
        host_arch = 'amd64'
    elif host_arch.startswith('arm'):
        host_arch = 'armhf'
    else:
        host_arch = "i386"
    return host_arch


def get_info(filename):
    """
        Read and parse info file
    """
    with open(filename, 'r') as f:
        info = json.load(f)
    return info


def create_control_file(dest, info, embed_deps=False):
    """
        Create the control file in DEBIAN with package information
        if embed_deps == False then add dependencies data
        if embed_deps == True then change the name to reflect that this is a full package
    """
    fdest = os.path.join(dest, "DEBIAN")
    try:
        shutil.rmtree(fdest)
    except:
        pass
    os.mkdir(fdest)
    with open(os.path.join(fdest, "control"),'w') as f:
        f.write("Package: %s\n" % info.get("name", "noname"))
        f.write("Version: %s\n" % info.get("version", "0.0.0-0"))
        f.write("Section: base\n" )
        if not embed_deps and info.get("dependencies"):
            f.write("Depends: %s\n" % info.get("dependencies"))
        f.write("Priority: optional\n" )
        f.write("Architecture: %s\n" % get_arch())
        f.write("Description: %s\n" % info.get("description", "sample package"))
        f.write("Maintainer: Nicolas Gargaud (Jacen) <jacen92gmail.com>\n" )
        f.write("Homepage: https://maison-gargaud.info\n" )


def copy_bin(dest, target):
    """
        Copy the project binary
    """
    fdest = os.path.join(dest, "bin")
    try:
        shutil.rmtree(fdest)
    except:
        pass
    os.mkdir(fdest)
    shutil.copy(target, fdest)


def copy_deps(dest, libraries):
    """
        Copy all the project libraries
    """
    fdest = os.path.join(dest, "lib")
    try:
        shutil.rmtree(fdest)
    except:
        pass
    os.mkdir(fdest)
    for lib in libraries:
        lpath = libraries[lib]
        if lpath:
            try:
                shutil.copy(lpath, fdest)
            except Exception, e:
                print "ERROR on %s: not found (%s)" % (lpath, e)
                raise e


def create_roots(roots):
    """
        Create the root directory for each version of the package
    """
    for root in roots:
        try:
            shutil.rmtree(root)
        except:
            pass
        os.mkdir(root)
        os.mkdir(os.path.join(root, "usr"))


def make_deb(pkgs):
    """
        Call dpkg to make the debian package
    """
    for pkg in pkgs:
        print "make %s" % pkg
        out = subprocess.check_output(["dpkg-deb", "--build", pkg])
        print out


def create_package(target, libraries, info=dict()):
    """
        Create package structure
    """
    assert type(info) == dict, "info must be a dictionnary"
    assert info.get("name") != None, "Name not specified in the information file"
    pkg = "%s-%s-%s" % (info.get("name"), get_arch(), info.get("version"))
    pkg_full = "%s-with_deps-%s-%s" % (info.get("name"), get_arch(), info.get("version"))
    create_roots([pkg, pkg_full])
    copy_bin(os.path.join(pkg, "usr"), target)
    copy_bin(os.path.join(pkg_full, "usr"), target)
    copy_deps(os.path.join(pkg_full, "usr"), libraries)
    create_control_file(pkg, info)
    create_control_file(pkg_full, info, True)
    make_deb([pkg, pkg_full])


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="pcb2gcode arguments helper when working with the laser.")
    parser.add_argument("--target", "-t", dest="target", help="name of the executable to package", default="../pcb2gcode")
    parser.add_argument("--info", "-i", dest="info", help="name of the information file", default="info.json")

    args = parser.parse_args()
    info = get_info(args.info)
    libraries = get_libraries(args.target)
    print json.dumps(libraries, indent=2)
    create_package(args.target, libraries, info)
