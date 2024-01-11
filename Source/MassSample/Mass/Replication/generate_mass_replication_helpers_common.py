def out(s, trimblanklines=False):
	if trimblanklines and ('\n' in s):
		lines = s.split('\n')
		if lines[0].strip() == '':
			del lines[0]
		if lines and lines[-1].strip() == '':
			del lines[-1]
		s = '\n'.join(lines)+'\n'
	print(s, end="")

def outl(s, **kw):
	out(s, **kw)
	out('\n')
	
# UE has an inconsistent naming where FTransformFragment is stored in FReplicatedAgentPositionYawData.
def replicated_data_getter(handler):
	return handler if handler != "Transform" else "PositionYaw"
