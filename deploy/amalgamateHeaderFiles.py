# -*- coding: utf-8 -*-
#
# This script merges all files in a C++ header-only library into a single header.
# © Lorenz Bucher, 2022. All rights reserved
# https://github.com/Sidelobe

from glob import glob
import sys
import os
import re

def strip_includes():
    return 1

def resolve_include_in_path(include_name, base_dir):
    """
    Tries to build an absolute file name based on an include_name, which may or 
    may not contain subdirectories it its syntax (e.g. "subdir/header.hpp").
    - Returns the absolute file name
    - Files in all subdirectories of the base path are 
    - In case of multiple matches, more precise matches are preferred
    """
    # split include name into path & file name
    incl_dir_file = os.path.split(include_name)
    if incl_dir_file[0]:
        # include contains a path: try direct resolution first
        include_file_name = base_dir + "/" + include_name
        if os.path.exists(include_file_name):
            return include_file_name
    # otherwise, search in all (sub)directories
    include_name = incl_dir_file[1] # keep only the file name
    for x in os.walk(base_dir):
        dir = x[0]
        include_file_name = dir + "/" + include_name
        if os.path.exists(include_file_name):
            return include_file_name
    raise RuntimeError("Could not find file for include: %s" % include_name)    
    
def extract_user_includes(file_name, base_dir, unique_includes):
    """
    returns a list of "user includes", ignores any <system includes>
    """

    with open(file_name) as f:
        contents = f.read()
        
    # Matches user includes: #include "MyHeader.hpp"
    user_include_pattern = re.compile(r'#include\s+\"(.+)\"')
    
    for (user_include) in re.findall(user_include_pattern, contents):
        user_include_file = resolve_include_in_path(user_include, base_dir)
        if not user_include_file in unique_includes:
            print "Include: \"%s\" ... found %s" %(user_include, user_include_file)
        unique_includes.add(user_include_file)
        extract_user_includes(user_include_file, base_dir, unique_includes)        

def main():
    """
    Merges all user includes in a provided C++ file into a single file.
    It tries to resolve the includes by recursively searching in subdirectories
    of the provided top-level include file.
    
    usage:  amalgamate.py topLevelInclude.hpp
    output: topLevelInclude_amalgamated.hpp
    """
    
    # open supplied top include and search for non-system #includes
    #
    # replace each #include with file contents:
    #    - strip any header notices and system includes
    
    if len(sys.argv) < 2:
        raise SyntaxError("Not enough arguments! Must provide top-level include .hpp")
    
    file_name = sys.argv[1]
    base_dir = os.path.dirname(os.path.abspath(file_name))
    print "\n   AMALGAMATOR    "
    print "========================\nScanning file %s for user includes " % file_name
    print "Resolving #includes in subdirectories of %s\n" % base_dir
    
    unique_includes = set()
    extract_user_includes(file_name, base_dir, unique_includes)
    
    print "\nFound chain of %d unique includes\n" % len(unique_includes)
    
    
    
if __name__ == "__main__":
    # execute only if run as a script
    main()
