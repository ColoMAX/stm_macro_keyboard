class bcolors:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKCYAN = '\033[96m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'

def cprint(string, color):
    print(color +string+bcolors.ENDC)
cprint("Macro upload script is active!",bcolors.WARNING)
cprint("If the build process seems to hang, look for notifications from the macro upload script.", bcolors.WARNING)
