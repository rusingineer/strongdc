#pragma once
#include <natupnp.h>

class UPnP
{
public:
	UPnP( const string, const string, const string, const short );
	~UPnP();
	HRESULT OpenPorts();
	HRESULT ClosePorts();
	string UPnP::GetExternalIP();
private:
	int PortNumber; // The Port number required to be opened
	BSTR bstrInternalClient; // Local IP Address
	BSTR bstrDescription;  // name shown in UPnP interfece details
	BSTR bstrProtocol; // protocol (TCP or UDP)
	BSTR bstrExternalIP; // external IP address
	IUPnPNAT* pUN;  // pointer to the UPnPNAT interface
	IStaticPortMappingCollection* pSPMC; // pointer to the collection
	IStaticPortMapping * pSPM; // pointer to the port map
};
