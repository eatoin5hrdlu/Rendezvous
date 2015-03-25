UD_DOWN = 1
UD_UP   = 2

def setup(pin, in_out, flags) :
    print "Pin " + str(pin) + " set for " + direction(in_out)

def direction(in_out) :
    if (in_out == UD_DOWN) :
        return " pull down "
    elif (in_out == UD_UP) :
        return " pull up "
    else :
        return " unknown "
