import subprocess

header_proc = subprocess.Popen(['python', 'generate_mass_replication_helpers_h.py'], stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
header_output = header_proc.communicate()[0]

with open("MassReplicationHelpersGenerated.h", "wb") as text_file:
    text_file.write(header_output)

cpp_proc = subprocess.Popen(['python', 'generate_mass_replication_helpers_cpp.py'], stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
cpp_output = cpp_proc.communicate()[0]

with open("MassReplicationHelpersGenerated.cpp", "wb") as text_file:
    text_file.write(cpp_output)
