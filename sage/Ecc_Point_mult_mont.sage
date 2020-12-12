def mont(*args):
    res = []
    for n in args:
        tres = n*rpoly
        res.append(tres)
    return res

def de_mont(*args):
    res = []
    for n in args:
        tres = n*rinvpoly
        res.append(tres)
    
    return res

apoly_mont = mont(apoly)[0]
bpoly_mont = mont(bpoly)[0]

def aff_to_jac(r_x,r_y):
    return r_x,r_y,rpoly

def jac_to_aff(r_x,r_y,r_z):
    #r_z_inv = r_z^exp*rpoly #works somehow
    #r_z_inv = r_z^-1
    r_z_inv = r_z^exp*rpoly
    r_z2_inv = r_z_inv*r_z_inv*rpoly
    r_z3_inv = r_z2_inv*r_z_inv
    
    r_x_a = r_x*r_z2_inv*rinvpoly
    r_y_a = r_y*r_z3_inv*rinvpoly
    
    return r_x_a,r_y_a

def point_add_jac_mont(X1,Y1,Z1,X2,Y2,Z2):
    
    O1 = Z1*Z1*rinvpoly
    O2 = Z2*Z2*rinvpoly

    A = X1*O2*rinvpoly
    B = X2*O1*rinvpoly

    T1 = Y1*O2*rinvpoly
    C = T1*Z2*rinvpoly

    T2 = Y2*O1*rinvpoly
    D = T2*Z1*rinvpoly

    E = A+B

    F = C+D

    G = E*Z1*rinvpoly

    T3 = F*X2*rinvpoly
    T4 = G*Y2*rinvpoly
    H = T3+T4

    Z3 = G*Z2*rinvpoly

    I = F+Z3

    T5 = Z3*Z3*rinvpoly
    T6 = apoly_mont*T5*rinvpoly
    T7 = F*I*rinvpoly
    T8 = T6+T7
    T9 = E*E*rinvpoly
    T10 = T9*E*rinvpoly
    X3 = T8+T10

    T11 = I*X3*rinvpoly
    T12 = G*G*rinvpoly
    T13 = T12*H*rinvpoly
    Y3 = T11+T13
    
    return X3,Y3,Z3

def point_double_jac_mont(X1,Y1,Z1):
    
    A = X1*X1*rinvpoly
    B = A*A*rinvpoly
    C = Z1*Z1*rinvpoly
    D = C*C*rinvpoly
    
    T1 = D*D*rinvpoly
    T2 = bpoly_mont*T1*rinvpoly
    X3 = B+T2
    
    Z3 = X1*C*rinvpoly
    
    T3 = B*Z3*rinvpoly
    T4 = Y1*Z1*rinvpoly
    T5 = A+T4
    T6 = T5+Z3
    T7 = T6*X3*rinvpoly
    Y3 = T3+T7

    return X3,Y3,Z3

def point_mult_jac_mont(X1,Y1,Z1,k):
    R_X = X1
    R_Y = Y1
    R_Z = Z1

    k_bin = bin(k)[3:]
    #print(bin(k),"\n",k_bin)
    for x in k_bin:
        R_X,R_Y,R_Z = point_double_jac_mont(R_X,R_Y,R_Z)#j
        if(x=="1"):
            R_X,R_Y,R_Z = point_add_jac_mont(R_X,R_Y,R_Z,X1,Y1,Z1)#j
            
    return R_X,R_Y,R_Z


# ## Testing Point addition
print("\n\n\n \t\t\t HELLO !!!")
print("\n\n\n1. Testing point addition")

P = E((str_to_poly("5973f2ce9481d19643a811a897599e6752b62834"), str_to_poly("655ebc626a619726018b03f84730e81a619142c94")))
Q = E((str_to_poly("525bd3cdda7c989f21652da0de9c7a5d35ae62f1f"), str_to_poly("54972dfe42d888584d8cfce0757f7b0a64801ae68")))


# ##### Montgomeraizing points

P_MD = mont(P.xy()[0],P.xy()[1],Q.xy()[0],Q.xy()[1])

assert P_MD[3] == Q.xy()[1]*rpoly


# ##### Affine to jacobi

P_MD_jac_x, P_MD_jac_y, P_MD_jac_z = aff_to_jac(P_MD[0],P_MD[1])

Q_MD_jac_x, Q_MD_jac_y, Q_MD_jac_z = aff_to_jac(P_MD[2],P_MD[3])


# ##### Addition

R_MD_jac_x, R_MD_jac_y, R_MD_jac_z = point_add_jac_mont(P_MD_jac_x, P_MD_jac_y, P_MD_jac_z, Q_MD_jac_x, Q_MD_jac_y, Q_MD_jac_z)


# ##### Jacobi to Affine

R_MD_x, R_MD_y = jac_to_aff(R_MD_jac_x, R_MD_jac_y, R_MD_jac_z)


# ##### Demontgomerizing Points

R = de_mont(R_MD_x,R_MD_y)

# ##### Result and varification

P_add = P+Q
print("P+Q by Sage:")

print(poly_to_str(P_add.xy()[0]),poly_to_str(P_add.xy()[1]))

print("P+Q by Our Function:")
print(poly_to_str(R[0]),poly_to_str(R[1]))

#Demont then jac - affine

#Ra = de_mont(R_MD_jac_x, R_MD_jac_y, R_MD_jac_z)


#Ra_x, Ra_y = jac_to_aff(Ra[0], Ra[1], Ra[2])

#print(poly_to_str(Ra_x),poly_to_str(Ra_y))

# ## Testing Point Doubling

print("\n\n2. Testing Point Doubling...")
D = P

# ##### Montgomerizing

D_MD = mont(D.xy()[0],D.xy()[1])
assert D_MD[1] == D.xy()[1]*rpoly


# ##### Affine to jacobi

D_MD_jac_x, D_MD_jac_y, D_MD_jac_z = aff_to_jac(D_MD[0],D_MD[1])


# ##### Doubling

R_D_MD_jac_x, R_D_MD_jac_y, R_D_MD_jac_z = point_double_jac_mont(D_MD_jac_x, D_MD_jac_y, D_MD_jac_z)


# ##### Jacobi to Affine

R_D_MD_x, R_D_MD_y = jac_to_aff(R_D_MD_jac_x, R_D_MD_jac_y, R_D_MD_jac_z)


# ##### Demontgomerizing Points

R_D = de_mont(R_D_MD_x, R_D_MD_y)


# ##### Result and varification

D_dub = 2*D
print("2*D by Sage:")
print(poly_to_str(D_dub.xy()[0]),poly_to_str(D_dub.xy()[1]))
print("2*D by Our function:")
print(poly_to_str(R_D[0]),poly_to_str(R_D[1]))


# ## Testing Point Multiplication
print("\n\n3. Testing Point Multiplication...")
M = P

I = Integer(getrandbits(PRECISION-1))
print("The Legendary Random Number is:",I)


# ##### Montgomerizing

M_MD = mont(M.xy()[0],M.xy()[1])
assert M_MD[1] == M.xy()[1]*rpoly


# ##### Affine to jacobi

M_MD_jac_x, M_MD_jac_y, M_MD_jac_z = aff_to_jac(M_MD[0],M_MD[1])


# ##### Doubling

R_M_MD_jac_x, R_M_MD_jac_y, R_M_MD_jac_z = point_mult_jac_mont(M_MD_jac_x, M_MD_jac_y, M_MD_jac_z,I)


# ##### Jacobi to Affine

R_M_MD_x, R_M_MD_y = jac_to_aff(R_M_MD_jac_x, R_M_MD_jac_y, R_M_MD_jac_z)


# ##### Demontgomerizing Points

R_M = de_mont(R_M_MD_x, R_M_MD_y)


# ##### Result and varification

M_Mul = I*M

print("I*M by Sage:")
print(poly_to_str(M_Mul.xy()[0]),poly_to_str(M_Mul.xy()[1]))
print("I*M by Our fungciotn:")
print(poly_to_str(R_M[0]),poly_to_str(R_M[1]))

print("\n\n\n \t\t\t BYE !!!")