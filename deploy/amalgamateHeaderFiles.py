# -*- coding: utf-8 -*-
#
# This script merges all files in a C++ header-only library into a single header.
# © Lorenz Bucher, 2022. All rights reserved
# https://github.com/Sidelobe

from glob import glob
import sys
import os
import re

system_include_pattern = re.compile(r'#include\s+<(.+[^\.][^h])>') # Matches system includes: #include <vector>
system_c_include_pattern = re.compile(r'#include\s+<(.+\.h)>') # Matches system "C" includes: #include <stdio.h>
user_include_pattern = re.compile(r'#include\s+\"(.+)\"') # Matches user includes: #include "MyHeader.hpp"
include_pattern = re.compile(r'(#include\s+[<|\"].+[>|\"])') # Matches all includes

def get_content_without_includes(file_name):
    result = ""
    with open(file_name) as contents:
        for line in contents:
            if not include_pattern.search(line) and not line.startswith("#pragma once"):
                result += line
    return result

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
    returns a unique list of file names for  "user includes" 
    (resolved recursively, depth first). Ignores any <system includes>
    """
    with open(file_name) as f:
        contents = f.read()
        
    for (user_include) in re.findall(user_include_pattern, contents):
        user_include_file = resolve_include_in_path(user_include, base_dir)
        # recursive call (depth first)
        extract_user_includes(user_include_file, base_dir, unique_includes)
        
        if not user_include_file in unique_includes:
            print "Include: \"%s\" ... found %s" %(user_include, user_include_file)
            unique_includes.append(user_include_file)
        
def extract_system_includes(file_names, unique_system_includes, unique_system_c_includes):
        """
        returns a unique list of <system includes> contained in all file_names
        """
        for file_name in file_names:
            with open(file_name) as f:
                contents = f.read()
            for (system_include) in re.findall(system_include_pattern, contents):
                if not system_include in unique_system_includes:
                    print "System include: <%s>" %  system_include
                    unique_system_includes.append(system_include)
            for (system_include) in re.findall(system_c_include_pattern, contents):
                if not system_include in unique_system_c_includes:
                    print "System \"C\" include: <%s>" %  system_include
                    unique_system_c_includes.append(system_include)
        
def main():
    """
    Merges all user includes in a provided C++ file into a single file.
    It tries to resolve the includes by recursively searching in subdirectories
    of the provided top-level include file.
    
    usage:  amalgamate.py topLevelInclude.hpp
    output: topLevelInclude_amalgamated.hpp
    """
    if len(sys.argv) < 2:
        raise SyntaxError("Not enough arguments! Must provide top-level include .hpp")
    
    file_name = sys.argv[1]
    base_dir = os.path.dirname(os.path.abspath(file_name))
    
    file_name_ext = os.path.splitext(file_name)
    out_file_name = file_name_ext[0] + "_amalgamated" + file_name_ext[1]
    
    print "\n   AMALGAMATOR    "
    print "========================\nScanning file %s for user includes " % file_name
    print "Resolving #includes in subdirectories of %s\n" % base_dir
    
    unique_user_includes = []
    extract_user_includes(file_name, base_dir, unique_user_includes)
    print "\nFound chain of %d unique user includes\n" % len(unique_user_includes)
    
    unique_system_includes = []
    unique_system_c_includes = []
    extract_system_includes(unique_user_includes, unique_system_includes, unique_system_c_includes)
    print "\nFound chain of %d unique system includes" % len(unique_system_includes)
    print "Found chain of %d unique system \"C\" includes\n" % len(unique_system_c_includes)
    unique_system_includes = sorted(unique_system_includes)
    unique_system_c_includes = sorted(unique_system_c_includes)
    
    # Replace all includes in top-level file with:
    # - list of system includes
    # - user include file contents
    includesAdded = False
    with open(file_name, "r") as original, open(out_file_name, "w") as amalgam:
        amalgam.write("// NOTE: This file is an amalgamation of individual source files to create a single-header include\n\n")
        
        for line in original:
            if not include_pattern.search(line):
                amalgam.write(line) # transfer line to output file
            elif user_include_pattern.search(line) and not includesAdded:
                # Add system includes before the first user include
                for s in unique_system_includes:
                    amalgam.write("#include <" + s + ">\n")
                amalgam.write("\nextern \"C\" \n{\n")
                for s in unique_system_c_includes:
                    amalgam.write("#include <" + s + ">\n")
                amalgam.write("}\n")    
                
                # add user includes
                for user_include_file in unique_user_includes:
                    include_file_contents = get_content_without_includes(user_include_file)
                    amalgam.write("\n// MARK: -------- " + os.path.relpath(user_include_file, base_dir) + " --------\n")
                    
                    # if include is a C file, put in 'extern "C"{}''
                    user_include_ext = os.path.splitext(user_include_file)
                    if user_include_ext[1] == ".h":
                        amalgam.write("extern \"C\" \n{\n")
                        amalgam.write(include_file_contents)
                        amalgam.write("} // extern \"C\"\n")
                    else:
                        amalgam.write(include_file_contents)
                includesAdded = True
                
    print "Created amalgamated header file in %s" % out_file_name
    
if __name__ == "__main__":
    # execute only if run as a script
    main()
