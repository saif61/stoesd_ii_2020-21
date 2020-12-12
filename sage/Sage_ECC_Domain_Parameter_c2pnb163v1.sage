PRECISION = 163

F = GF(2^163, 'g', modulus = x^163 + x^8 + x^2 + x + 1)
F.inject_variables()
R.<x,y> = F[]

def str_to_poly(str):
	I=Integer(str, base=16)
	v=F(0)
	for i in range (0,F.degree()):
		if(I >> i) & 1 > 0:
			v = v + g^i
	return v

def poly_to_str(poly):
	vec=poly._vector_()
	string = ""
	for i in range(0,len(vec)):
		string = string + str(vec[len(vec) - i - 1])
	return hex(Integer(string, base=2))

rpoly = str_to_poly("0107")

r = poly_to_str(rpoly)

rinvpoly = rpoly^-1

rinv = poly_to_str(rinvpoly)

r2poly = rpoly*rpoly

r2 = poly_to_str(r2poly)

n = 0x080000000000000000000000000000000000000107

a = 0x072546b5435234a422e0789675f432c89435de5242

apoly = str_to_poly("072546b5435234a422e0789675f432c89435de5242")

b = 0x00c9517d06d5240d3cff38c74b20b6cd4d6f9dd4d9

bpoly = str_to_poly("00c9517d06d5240d3cff38c74b20b6cd4d6f9dd4d9")

exp = (2^163) - 2

exp2 = 2^(163-2)

E = EllipticCurve(y^2 + x*y - x^3 - apoly*x^2 - bpoly)

G = E((str_to_poly("07af69989546103d79329fcc3d74880f33bbe803cb"), str_to_poly("01ec23211b5966adea1d3f87f7ea5848aef0b7ca9f")))

