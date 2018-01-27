import subprocess

for f in ["do","re","mi","fa","sol","la","si"]:
    print(f)
    #subprocess.call(["wav2vvd",f+".wav",f+".vvd"])
    #subprocess.call(["vvd2wav",f+".vvd","synth_"+f+".wav"])
    subprocess.call(["aplay","synth_"+f+".wav"])
    
