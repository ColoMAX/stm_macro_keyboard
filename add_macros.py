Import("env")

# access to global construction environment
#print(env)

# Dump construction environment (for debug purpose)
#print(env.Dump())

# def to_matrix(l, n):
#     return [l[i:i+n] for i in range(0, len(l), n)]

keys = list(range(1, 4))+['A']\
        +list(range(4,7)) + ['B']\
     +list(range(7,10))+['C']+\
     list('*0#D')

print('Welcome, this is the more secure macro input panel.\n\
    Please provide c-synthax for the strings (thus if e.g. enter key needs to be pressed, use \\n).\n\
         ')
print("If you wish to use the macro's specified in the header file, \
    disable this script in the platform.ini file.")
print('or type \"n\", otherwise type anything else: \t')
if(input()!='n'):


    print('The assumed kayboard layout is 4x4\n')

    # print('\n'.join([''.join(['{:4}'.format(item) for item in row]) 
    #       for row in to_matrix(keys, 4)]))

    for c, i in zip(keys, range(1, len(keys)+1)):
        #if(i-1%4==0):
        #    c='\t'+str(c)
        d=""
        
        if(i==1):
            d+="______________\n"

        d += str(c)
        if(i%4 !=0):
            d+=" | "
        if(i%4==0):
            d+='\n'+"______________\n"
        
        print(d, end='')

    print()
    value = []
    for i in keys:
        value.append(input(f'Please enter macro for the \"{i}\"-key:\n'))
    for m , k in zip(value, keys):
        print(f'{k} saved macro = {m}')


    CPPDEFS = ["USE_SCRIPT_MACROS "]

    for m, k in zip(value, keys):
        if(k=='#'):
            name="MACRO_H_NAME"
        elif(k=='*'):
            name="MACRO_S_NAME"
        else:
            name=f"MACRO_{k}_NAME"
        if type(m) is None:
            m=""
        CPPDEFS.append((name,m))

    env.Append(CPPDEFINES=CPPDEFS)
    # env.Append(CPPDEFINES=[
    #   "MACRO_1_NAME",
    #   ("MACRO_2_NAME", "MACRO_2_VALUE")
    # ])

    #print(env.Dump())
