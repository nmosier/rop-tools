BEGIN {
    reg1[-1] = ""; # index2reg
    reg2[-1] = ""; # index2reg
    ind1[""] = -1; # reg2index
    ind2[""] = -1; # reg2index
    cnt1 = 0;
    cnt2 = 0;
    FS="[ \t]|, "
    state = 0;
}

# state 0: nothing found
# state 1: found LEA
# state 2: found ADD | XOR | POP \\ ADD

/^$/ {
    cnt1 = 0;
    cnt2 = 0;
    state = 0;
    delete reg1;
    delete reg2;
    delete ind1;
    delete ind2;    
}

/lea\tr.., \[rip.*\]/ {
    for (i = 1; i <= NR; ++i) {
	if ($i == "lea") {
	    ++cnt1;
	    reg1[cnt1] = $(i+1);
	    ind1[$(i+1)] = cnt1;
	    break;
	}
    }
}

/mov\tr.., .word ptr \[.*r[sb]p.*\]/ {
    for (i = 1; i <= NR; ++i) {
	if ($i == "mov") {
	    val1 = ind1[$(i+1)];
	    if (val1 <= 0) {
		++cnt1;
		reg1[cnt1] = $(i+1);
		ind1[$(i+1)] = cnt1;
	    }
	}
    }
}

/add\tr.., .word ptr \[.*r[sb]p.*\]/ {
    for (i = 1; i <= NR; ++i) {
	if ($i == "add") {
	    val1 = ind1[$(i+1)];
	    if (val1 > 0 && val1 <= cnt1 && state == 0) {
		state = 1;
	    }
	    break;
	}
    }
}

/xor\tr.., .word ptr \[.&r[sb]p.*\]/ {
    for (i = 1; i <= NR; ++i) {
	if ($i == "xor") {
	    val1 = ind1[$(i+1)];
	    if (val1 > 0 && val1 <= cnt1 && state == 0) {
		state = 1;
	    }
	    break;
	}
    }
}

/pop/ {
    for (i = 1; i <= NF; ++i) {
	if ($i == "pop") {
	    ++cnt2;
	    reg2[cnt2] = $(i+1);
	    ind2[$(i+1)] = cnt2;
	    break;
	}
    }
}

/add\tr.., r../ {
    for (i = 1; i <= NF; ++i) {
	if ($i == "add") {
	    #valXY where X is the register #, Y is the argument #
	    val11 = ind1[$(i+1)];
	    val12 = ind1[$(i+2)];
	    val21 = ind2[$(i+1)];
	    val22 = ind2[$(i+2)];

	    if ((val11 > 0 && val11 <= cnt1 && val22 > 0 && val22 <= cnt2) || (val12 > 0 && val21 <= cnt1 && val21 > 0 && val21 <= cnt2) && state == 0) {
		state = 1;
	    }
	}
    }
}

/mov\tr.., r../ {
    for (i = 1; i <= NF; ++i) {
	if ($i == "mov") {
	    rhs1 = ind1[$(i+2)];
	    rhs2 = ind2[$(i+2)];
	    lhs1 = ind1[$(i+1)];
	    lhs2 = ind2[$(i+1)];
	    if (rhs1 > 0 && rhs1 <= cnt1 && lhs1 < 0) {
		++cnt1;
		reg1[cnt1] = $(i+1);
		ind1[$(i+1)] = cnt1;
	    } else if (rhs2 > 0 && rhs2 <= cnt2 && lhs2 < 0) {
		++cnt2;
		reg2[cnt2] = $(i+1);
		ind2[$(i+1)] = cnt2;
	    }
	    break;
	}
    }
}

{
    if (state == 1) {
	state = 2;
	print NR;
    }

    #
    #if (NR % (int(LEN/100)) == 0) {
    #print "[" int((NR/LEN)*100) "%]";
#}
}
