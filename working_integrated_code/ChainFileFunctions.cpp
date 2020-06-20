/*
Functions used for the chain file.


These function definations in this file:

1. STACK_OF(X509) * getCertStackFromPath (string certStackFilepath);
2. string getSerialNumberFromX509(X509 *input);
3. STACK_OF(X509) * correctCertStackOrder(STACK_OF(X509) *certStack);
4. void printCertChainSerialNumbers(vectot<string> chainFileSerialNumbers)
*/

#include "Common.h"
#include "ChainFileFunctions.h"
#include <string.h> // For using strcpy.

using namespace std;

STACK_OF(X509) * getCertStackFromPath(string certStackFilepath)
{

	// Convert filepath to a C-style string, this is needed for openssl.
    char filePath[certStackFilepath.length() + 1];
    strcpy(filePath, certStackFilepath.c_str());

    SSL_CTX *sslCtx = SSL_CTX_new(SSLv23_server_method());
    if(sslCtx == NULL)
    {
        cerr << "Failed to create SSL_CTX object." << endl;
        exit(-1);
    }

    int result = SSL_CTX_use_certificate_chain_file(sslCtx, filePath);
    if(result != 1)
    {
        cerr << "Failed to load certificates into the SSL_CTX object." << endl;
        exit(-1);
    }

    STACK_OF(X509) * tempCertStack;
    STACK_OF(X509) * certStack;
    X509 *leaf;
    int num;

    // Get the certs from sslCtx into temp_stack
    if (SSL_CTX_get0_chain_certs(sslCtx, &tempCertStack) == 0)
    {
        cout << "Error in getting stack from SSL_CTX" << endl;
        exit(-1);
    }

    // Print the leaf cert
    leaf = SSL_CTX_get0_certificate(sslCtx);
    if(leaf == NULL)
    {
        cout << "Failed to get the active certificate from SSL_CTX" << endl;
        exit(-1);
    }

    // Create a copy of the stack
    certStack = X509_chain_up_ref(tempCertStack); // This increases the referencability of tempCertStack by 1, and assigns it to certStack. Now, even if certStack is freed, leaf will continue to function.
    if (certStack == NULL)
    {
        cout << "Error creating copy of stack" << endl;
        exit(-1);
    }

    result = X509_up_ref(leaf); // This increases the referencability of leaf by 1. Now, even if sslCtx is freed, leaf will continue to function.
    if (certStack == NULL)
    {
        cout << "Failed to increment the reference count of the X509* vaariable." << endl;
        exit(-1);
    }

    //Insert the leaf cert into stack
    num = sk_X509_insert(certStack, leaf, 0);
    if (num == 0)
    {
        cout << "Error inserting leaf cert into stack" << endl;
        exit(-1);
    }
    // cout<<"Number of certs in stack = "<<num<<endl;

    SSL_CTX_free(sslCtx);

    return certStack;
}


string getSerialNumberFromX509(X509 *input)
{
    return convertASN1ToString(X509_get_serialNumber(input));
}


/*
We implicitly assume that the user will provide certificates of either one of the two orders :
1. leaf, intermediates, root
2. root, intermediates, leaf

The correct order required for the program to work is :
leaf , intermediates, root

This function changes the order to the correct one
*/
STACK_OF(X509) * correctCertStackOrder(STACK_OF(X509) *certStack)
{
    X509 *firstCert = sk_X509_value(certStack, 0);

    // Implicitly assumes root is at the beginning
    if(X509_check_ca(firstCert) == 1)
    {
        // Allocate a new stack of the same size as the original
        int stackSize = sk_X509_num(certStack);
        STACK_OF(X509) *newCertStack = sk_X509_new_reserve(NULL, stackSize);
        if(newCertStack == NULL)
        {
            std::cerr<<"Error creating new X509 stack"<<std::endl;
            exit(-1);
        }

        // Insert the certs into the new stacks in the reverse order
        for(int i = stackSize -1; i >= 0; i--)
        {
            sk_X509_push(newCertStack, sk_X509_value(certStack, i));
        }

        sk_X509_free(certStack);

        return newCertStack;
    }
    else {
        return certStack;
    }
}


void printCertChainSerialNumbers(vector<string> chainFileSerialNumbers)  // Display all serial numbers in the chain file.
{
    cout << "\nThese are the serial numbers in the chain file:" << endl;
    for (int i = 0; i < chainFileSerialNumbers.size(); i++)
    {
        cout << (i + 1) << ". " << chainFileSerialNumbers[i] << endl;
    }
    cout << endl;
}
