This project is a part of the course Selected Topics of Embedded Software Development II.

## introduction
Elliptic Curve Cryptography (ECC) defines asymmetric cryptographic systems, which perform
operations on elliptic curves over finite fields. Such methods are secure only if discrete
logarithms in the group of points of the elliptic curve cannot be efficiently calculated. The
computation on elliptic curves is done by the operations Point Addition, Point Doubling and
Point Multiplication in which the latter is based on a sequence of Point Doublings and Point
Additions. Since the problem statement of solving discrete logarithms on elliptic curves
(ECDLP) is much harder than the calculation of discrete logarithms in finite fields or the
factorization of integers, elliptic curve cryptographic systems - at the same level of security -
work with considerable shorter key lengths compared to conventional asymmetric cryptographic
systems such as RSA and the Diffie-Hellman key exchange. The calculation rule of an ECC Point
Addition as well as Point Doubling in affine coordinate representation requires
division-operations (thus the calculation of a multiplicative inverse element) which is a quite
time-consuming calculation. For a time-efficient calculation the transformation into the
projective jacobian coordinates has proven itself, since no division operation is required here in
the named operations. Though, this method requires a back transformation step in which a
multiplicative inverse element must be calculated, but thin only has to be performed once at the
very end of an ECC computation.

#### This project is implemented on a FPGA device, dedicated for cryptography, called "CryptoCore", using it's linux based driver.

## Implementation
The ip address of the cryptocore assigned to us is 192.168.101.211. To connect from a local
machine, first we have to connect to the THD’s VPN using the provided application “Cisco
anyconnect”. To connect from a local linux machine using the following command.

ssh root@192.168.101.211

After connecting, we can navigate to our project folder using the following command.
cd stoesd_ii_2020-21/
Here is a short description of interesting folders.
● driver/ - contains the cryptocore_driver.c file which produces the linux driver module for
the cryptocore.
● include/ - contains header file cryptocore_ioctl_header.h
● application/binary/ - contains applications for ECC over the binary extension field. Our
application is PointMult.c

### Compiling and running
To run the project, we first build the driver code. We use the make and clean command. Then
we load our driver module to the linux kernel. After that we compile our application (PointMult.c).
It will produce a binary file, which we can run using ./ command. In details:

To build the driver code.

make clean && make

To load the driver module cryptocore_driver.ko.

insmod cryptocore_driver.ko

To list all loaded modules, use:

lsmod

To remove driver module cryptocore_driver.ko, use:

rmmod cryptocore_driver.ko

To compile the application, use:

gcc PointMult.c -o output -lrt

It will produce a file named output, which we can run by following command, use:

./output

The output shows the elements of cryptocore memory ie. B1, P1, X7 etc. First output will show the computation before post operation(converting to affine coordinates and
de-montgomarizing). The coordinates(x,y,z) are written in X7,X5,X3 respectively. The second
part of the output shows the final result(x,y), which are written in B1,B3 respectively.
