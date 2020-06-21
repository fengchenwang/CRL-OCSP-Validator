// sudo apt-get install libssl-dev

/*

g++ -c -o Common.o Common.cpp
g++ -c -o ChainFileFunctions.o ChainFileFunctions.cpp
g++ -c -o CRLFunctions.o CRLFunctions.cpp
g++ -c -o OCSPFunctions.o OCSPFunctions.cpp
g++ -c -o application.o application.cpp
g++ -o application Common.o ChainFileFunctions.o CRLFunctions.o OCSPFunctions.o application.o -lcrypto -lssl
./application


g++ -c -o Common.o Common.cpp && \
g++ -c -o ChainFileFunctions.o ChainFileFunctions.cpp && \
g++ -c -o CRLFunctions.o CRLFunctions.cpp && \
g++ -c -o OCSPFunctions.o OCSPFunctions.cpp && \
g++ -c -o application.o application.cpp && \
g++ -o application Common.o ChainFileFunctions.o CRLFunctions.o OCSPFunctions.o application.o -lcrypto -lssl && \
./application

*/

#include "Common.h"
#include "ChainFileFunctions.h"
#include "CRLFunctions.h"
#include "OCSPFunctions.h"


std::string checkIfFileHasBeenDraggedIn(std::string originalPath) // If the file has been dragged into the console, single quotes will be present at both the start and end of the string, which have to be removed.
{
	std::string changedPath = originalPath;

	if (changedPath[0] != '/') // Yes, the file has been dragged and dropped into the console.
	{
		// Remove the first and last chars.
		changedPath.erase(changedPath.begin() + 0);
		changedPath.erase(changedPath.end() - 1);
	}
	return changedPath;
}

int main()
{
	OpenSSL_add_all_algorithms();
	ERR_load_BIO_strings();

	// Display intro message.

	std::cout << "\nThis is a tool to validate a given certificate chain file against a given CRL.\n"
			  << std::endl;
	std::cout << "Built as an in-semester project by Pranav Kulkarni and Teja Juluru. Mentored by Prof Rajesh Gopakumar and HPE Technologist Vicramaraja ARV.\n"
			  << std::endl;
	std::cout << "----------------------------------------------------------------\n"
			  << std::endl;

	// Get the path of the certificate chain file

	std::cout << "Enter the full path of the certificate chain file. ";
	std::cout << "Alternatively, drag and drop the file into this terminal window." << std::endl;
	std::string certChainFilePath;
	std::cin >> certChainFilePath;

	certChainFilePath = checkIfFileHasBeenDraggedIn(certChainFilePath);

	std::vector<std::string> chainFileSerialNumbers;

	STACK_OF(X509) *certStack = getCertStackFromPath(certChainFilePath); //Get the stack of certificates from the path.

	certStack = correctCertStackOrder(certStack); // Correct the order of the certificate stack

	int numberOfCertificatesInChain = sk_X509_num(certStack); // Get the number of certificates in the chain file.

	for (int i = 0; i < numberOfCertificatesInChain; i++)
	{
		X509 *thisCert = sk_X509_value(certStack, i);						 // Pick one cert from the stack.
		chainFileSerialNumbers.push_back(getSerialNumberFromX509(thisCert)); // Add the serial number to the chainFileSerialNumbers vector.
	}

	// We now have all chain file's serial numbers in the string vector chainFileSerialNumbers.

	// We can now compare these againt the serial numbers in the CRL file.

	// As the CRl files usually contains thousands of entries, better to put them in a map for O(1) lookup when we iterate through chainFileSerialNumbers while cheking.

	// Get the CRL file path from the user.

	std::cout << "\n\nEnter the full path of the CRL file. ";
	std::cout << "Alternatively, drag and drop the file into this terminal window." << std::endl;
	std::string CRLFilePath;
	std::cin >> CRLFilePath;

	CRLFilePath = checkIfFileHasBeenDraggedIn(CRLFilePath);

	X509_CRL *CRLFileInX509 = getNewCRLFromPath(CRLFilePath);

	STACK_OF(X509_REVOKED) *revokedStack = X509_CRL_get_REVOKED(CRLFileInX509); // Get the stack of revoked certificates.

	int numberOfRevokedCeritficates = sk_X509_REVOKED_num(revokedStack); // Get the number of revoked certificates from the CRL.

	// Extract serial numbers of all revoked certificates, and puts it in a map for fast access.

	std::map<std::string, int> revokedSerialNumbers;
	X509_REVOKED *revStackEntry = NULL;

	for (int i = 0; i < numberOfRevokedCeritficates; i++)
	{
		revStackEntry = sk_X509_REVOKED_value(revokedStack, i);					 //Pick one from the stack.
		std::string thisSerialNumber = getRevokedSerialNumberFromX509(revStackEntry); // Extract it's serial number.

		revokedSerialNumbers[thisSerialNumber] = (i + 1); // Add its index to the revokedSerialNumbers map. (1 - indexed)
	}

	// Now we have one vector (chainFileSerialNumbers) and one map (revokedSerialNumbers), with all the required serial numbers.

	// Do the checking. That is, see if there is any cert from the chain file which is listed in the CRL. If there is, the chain file is NOT VALID.

	int CRLvalidityStatus = 0;						   // Let 0 be non-revoked and 1 be revoked.
	std::vector<std::string> CRLcertChainRevokedCerts; // All chain file certificates that are found to also exist in the CRL will be inserted here.

	for (int i = 0; i < chainFileSerialNumbers.size(); i++)
	{
		std::string toBeChecked = chainFileSerialNumbers[i];

		if (revokedSerialNumbers[toBeChecked] != 0) // If true, this cert exists in the CRL file.
		{
			CRLvalidityStatus = 1;							 // Set the status as revoked.
			CRLcertChainRevokedCerts.push_back(toBeChecked); // Add it to the list of revoked certs from the input chain file.
		}
	}

	if (CRLvalidityStatus == 1) // Revoked
	{
		std::cout << "\nChain file is INVALID because the following certificates from the chain file were found to be listed in the CRL." << std::endl;
		for (int i = 0; i < CRLcertChainRevokedCerts.size(); i++)
		{
			std::string thisCert = CRLcertChainRevokedCerts[i];
			std::cout << (i + 1) << ". " << thisCert << " was found at index " << revokedSerialNumbers[thisCert] << "." << std::endl;
		}
		std::cout << "\n\n";
	}
	else // Non-revoked
	{
		std::cout << "\nChain file is VALID because none of the certificates from the chain file were found to be listed in the CRL.\n"
				  << std::endl;
	}

	//===============================================
	// Code for OCSP checking starts here.

	int OCSPvalidityStatus = 0;							// Let 0 be non-revoked and 1 be revoked.
	std::vector<std::string> OCSPcertChainRevokedCerts; // All certs found to be revoked by OCSP will be inserted here.

	for (int i = 0; i < sk_X509_num(certStack) - 1; i++) // Going through all certs of the chain file.
	{

		X509 *thisCert = NULL, *thisCertIssuer = NULL;

		thisCert = sk_X509_value(certStack, i);
		thisCertIssuer = sk_X509_value(certStack, i + 1);

		std::vector<std::string> ocspURLs = getocspURLs(thisCert); // Get all URLs as a std::vector<string>.

		for (std::string thisURL : ocspURLs) // Iterate through all provided URLs until one of them responds.
		{

			OCSP_CERTID *certID = getCertificateID(thisCert, thisCertIssuer);

			OCSP_REQUEST *thisRequest = createOCSPRequest(certID, thisURL);

			// Parse the URL.
			char *host = NULL, *port = NULL, *path = NULL;
			int useSSL;
			if (OCSP_parse_url(thisURL.c_str(), &host, &port, &path, &useSSL) == 0)
			{
				std::cerr << "Failed to parse URL." << std::endl;
				exit(-1);
			}

			// Create the connection BIO.
			BIO *connBIO = NULL;
			connBIO = BIO_new_connect(host);
			if (connBIO == NULL)
			{
				std::cerr << "Error creating connection BIO" << std::endl;
				exit(-1);
			}

			BIO_set_conn_port(connBIO, port);

			//
			// OCSP_REQUEST_print(outbio, req, 0); debug function which is not working at the moment.
			//

			OCSP_REQ_CTX *requestCTX = createOCSPRequestCTX(connBIO, path, host);

			// Set the OCSP request.
			if (OCSP_REQ_CTX_set1_req(requestCTX, thisRequest) == 0)
			{
				std::cerr << "Error setting the OCSP request CTX object" << std::endl;
				exit(-1);
			}

			// Connect to the OCSP responder.
			if (BIO_do_connect(connBIO) <= 0)
			{
				std::cerr << "Error connecting to BIO" << std::endl;
				exit(-1);
			}

			// Request timeout handling goes here, impotant but can be done later.

			// Execute the connection.
			OCSP_RESPONSE *thisResponse = NULL;

			if (OCSP_sendreq_nbio(&thisResponse, requestCTX) == 0)
			{
				std::cerr << "Error occured in sending the request" << std::endl;
				exit(-1);
			}

			// Tear down the non-necessary structures.
			OCSP_REQ_CTX_free(requestCTX);
			BIO_free_all(connBIO);

			//
			// OCSP_RESPONSE_print(); ; debug function which is not working at the moment.
			//

			// Check the status of the certificate from the response.
			int returnedOcspStatus, returnedOcspReason;
			ASN1_GENERALIZEDTIME *revokedTime = NULL;
			getCertificateStatus(thisResponse, certID, &returnedOcspStatus, &returnedOcspReason, &revokedTime);

			OCSP_RESPONSE_free(thisResponse);
			OCSP_REQUEST_free(thisRequest); //Also frees certID.

			if (returnedOcspStatus == V_OCSP_CERTSTATUS_GOOD)
			{
				// This certificate is valid, so we can break out (of the URLs loop) and start validating other certs.
				break;
			}
			else if (returnedOcspStatus == V_OCSP_CERTSTATUS_REVOKED)
			{
				OCSPvalidityStatus = 1;													// Set the status as revoked.
				OCSPcertChainRevokedCerts.push_back(getSerialNumberFromX509(thisCert)); // Add it to the list of revoked certs from the input chain file.
				break; // Break out of the URLs loop and go on to check further certs.
			}
			else if (returnedOcspStatus == V_OCSP_CERTSTATUS_UNKNOWN)
			{
				std::cerr << "Certicate status is UNKNOWN by OCSP Server." << std::endl;
				// break; check if one URL returns "unkown status", do all other URLs also return unknown status????????
			}
			else
			{
				std::cerr << "Invalid OCSP status code received." << std::endl;
				exit(-1);
			}

		} // End of inner URL loop.

	} // End of of the loop going through the chain file.

	if (OCSPvalidityStatus == 1) // Revoked
	{
		std::cout << "\nChain file is INVALID because the following certificates from the chain file were returned as revoked from the OCSP server." << std::endl;
		for (int i = 0; i < OCSPcertChainRevokedCerts.size(); i++)
		{
			std::cout << (i + 1) << ". " << OCSPcertChainRevokedCerts[i] << std::endl;
		}
		std::cout << "\n\n";
	}
	else // Non-revoked
	{
		std::cout << "\nChain file is VALID because none of the certificates from the chain file were returned as revoked from the OCSP server.\n"
				  << std::endl;
	}

	return 0;
}