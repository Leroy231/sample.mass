import os

buffer = ""

def out(s, trimblanklines=False):
	global buffer
	if trimblanklines and ('\n' in s):
		lines = s.split('\n')
		if lines[0].strip() == '':
			del lines[0]
		if lines and lines[-1].strip() == '':
			del lines[-1]
		s = '\n'.join(lines)+'\n'
	buffer += s

def outl(s, **kw):
	out(s, **kw)
	out('\n')
	
def write_to_file(filename):
	global buffer
	script_path = os.path.abspath(__file__)
	script_dir = os.path.dirname(script_path)
	with open(os.path.join(script_dir, filename), "w") as text_file:
		text_file.write(buffer)
	buffer = ""

# UE has an inconsistent naming where FTransformFragment is stored in FReplicatedAgentPositionYawData.
def replicated_data_getter(handler):
	return handler if handler != "Transform" else "PositionYaw"

generated_file_header = "// THIS IS GENERATED CODE. DO NOT MODIFY.\n// REGENERATE WITH: python generate_mass_replication_helpers.py"

type_defaults = {"int32": "0", "bool": "false", "float": "0.f", "double": "0.0"}

def is_float(type):
	return type == "float" or type == "double"
