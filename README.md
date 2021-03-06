# CRL-OCSP-Validator
6th semester project offered by HP Enterprises on certificate chain validation using CRLs and OCSP.

## Instructions to install:
(for Ubuntu based systems):

After cloning the directory,

   #### Commands to run
   
0. Install the OpenSSL library 

   `sudo apt-get install libssl-dev`

1. Start the terminal and navigate to the root of the project. 

2. Make a build directory.

   ```mkdir build```

3. Go into into build directory.

   ```cd build```

4. Run cmake (CMakeLists.txt is in the root).

   ```cmake ..```

5. Run make.

   ```make```

6. The executable (called application) will be created in the root directory. Go to root.

   ```cd ..```

7. Run the executable.

   ```./application```

## Verbose mode:
Running the executable as ```./application -v``` will provide more information. This incudes: number of certificates in the chain file, number of certificates in the CRL file, number of certificates revoked by CRL method and number of certificates revoked by OCSP method.

## A note about the chain files:
For OCSP to function properly, the chain needs to be in the right order. It can be from leaf to root, or vice versa. 
We handle both these cases, and we assume there will be no input with jumbled order of certs.

## Test files:
We have provided 10 pairs of (chain file, CRL file) for testing. 5 of them returm VALID and the other 5 return REVOKED.
All CRL files have been provided in both PEM and DER formats.
These 10 pairs can be found in test/test_cert_files.

## Unit testing:
We have unit tested a few functions, and captured results as screenshots.
The code and the screenshots can be found in test/unit_testing.
To run the code, make a folder with all .cpp files (from 'src' directory) and all .h files (from 'include' directory).

Then, add the path to the input file to the string inside the test CPP files, and compile+run them using the command provided at the top of these files.
