import subprocess

import os

# Change directory to the location of this file
script_path = os.path.abspath(__file__)
script_dir = os.path.dirname(script_path)
os.chdir(script_dir)

import generate_mass_replication_helpers_cpp
import generate_mass_replication_helpers_h
