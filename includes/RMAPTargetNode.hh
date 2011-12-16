/*
 * RMAPTargetNode.hh
 *
 *  Created on: Aug 2, 2011
 *      Author: yuasa
 */

#ifndef RMAPTARGETNODE_HH_
#define RMAPTARGETNODE_HH_

#include <CxxUtilities/CommonHeader.hh>
#include "SpaceWireUtilities.hh"
#include "RMAPMemoryObject.hh"
#include "RMAPNode.hh"
#ifndef NO_XMLLODER
#include "XMLLoader.hpp"
#endif

class RMAPTargetNodeException: public CxxUtilities::Exception {
private:
	std::string errorFilename;
	bool isErrorFilenameSet_;

public:
	enum {
		FileNotFound, InvalidXMLEntry, NoSuchRMAPMemoryObject
	};

public:
	RMAPTargetNodeException(int status) :
		CxxUtilities::Exception(status) {
		isErrorFilenameSet_ = false;
	}

	RMAPTargetNodeException(int status, std::string errorFilename) :
		CxxUtilities::Exception(status) {
		setErrorFilename(errorFilename);
	}

	void setErrorFilename(std::string errorFilename) {
		this->errorFilename = errorFilename;
		isErrorFilenameSet_ = true;
	}

	std::string getErrorFilename() {
		return errorFilename;
	}

	bool isErrorFilenameSet() {
		return isErrorFilenameSet_;
	}

};

class RMAPTargetNode: public RMAPNode {
private:
	std::vector<uint8_t> targetSpaceWireAddress;
	std::vector<uint8_t> replyAddress;
	uint8_t targetLogicalAddress;
	uint8_t initiatorLogicalAddress;
	uint8_t defaultKey;

	bool isInitiatorLogicalAddressSet_;

public:
	static const uint8_t DefaultLogicalAddress = 0xFE;
	static const uint8_t DefaultKey = 0x20;

private:
	std::map<std::string, RMAPMemoryObject*> memoryObjects;

public:
	RMAPTargetNode() {
		targetLogicalAddress = 0xFE;
		initiatorLogicalAddress = 0xFE;
		defaultKey = DefaultKey;
		isInitiatorLogicalAddressSet_ = false;
	}

#ifndef NO_XMLLODER
public:
	static std::vector<RMAPTargetNode*> constructFromXMLFile(std::string filename) throw (XMLLoader::XMLLoaderException,RMAPTargetNodeException,RMAPMemoryObjectException) {
		using namespace std;
		ifstream ifs(filename.c_str());
		if(!ifs.is_open()) {
			throw RMAPTargetNodeException(RMAPTargetNodeException::FileNotFound);
		}
		ifs.close();
		XMLLoader xmlLoader(filename.c_str());
		std::vector<RMAPTargetNode*> result;
		try {
			XMLNode* topNode;
			topNode=xmlLoader.getTopNode();
			if(topNode!=NULL) {
				result=constructFromXMLFile(topNode);
				return result;
			} else {
				throw RMAPTargetNodeException(RMAPTargetNodeException::InvalidXMLEntry,filename);
			}
		} catch(RMAPTargetNodeException e) {
			if(e.isErrorFilenameSet()) {
				throw e;
			} else {
				throw RMAPTargetNodeException(RMAPTargetNodeException::InvalidXMLEntry,filename);
			}
		} catch(...) {
			throw RMAPTargetNodeException(RMAPTargetNodeException::InvalidXMLEntry,filename);
		}
	}

	static std::vector<RMAPTargetNode*> constructFromXMLFile(XMLNode* topNode) throw (XMLLoader::XMLLoaderException,RMAPTargetNodeException,RMAPMemoryObjectException) {
		using namespace std;
		if(topNode==NULL) {
			throw RMAPTargetNodeException(RMAPTargetNodeException::InvalidXMLEntry);
		}
		vector<XMLNode*> nodes = topNode->getChildren("RMAPTargetNode");
		vector<RMAPTargetNode*> result;
		for (unsigned int i = 0; i < nodes.size(); i++) {
			if(nodes[i]!=NULL) {
				try {
					result.push_back(RMAPTargetNode::constructFromXMLNode(nodes[i]));
				} catch(RMAPMemoryObjectException e) {
					throw e;
				} catch(RMAPTargetNodeException e) {
					if(e.isErrorFilenameSet()) {
						throw RMAPTargetNodeException(RMAPTargetNodeException::InvalidXMLEntry,e.getErrorFilename());
					} else {
						throw RMAPTargetNodeException(RMAPTargetNodeException::InvalidXMLEntry);
					}
				} catch(...) {
					throw RMAPTargetNodeException(RMAPTargetNodeException::InvalidXMLEntry);
				}
			} else {
				throw RMAPTargetNodeException(RMAPTargetNodeException::InvalidXMLEntry);
			}
		}
		return result;
	}

	static RMAPTargetNode* constructFromXMLNode(XMLNode* node) throw (XMLLoader::XMLLoaderException,RMAPTargetNodeException,RMAPMemoryObjectException) {
		using namespace std;
		using namespace CxxUtilities;

		const char* tagNames[] = {"TargetLogicalAddress", "TargetSpaceWireAddress", "ReplyAddress", "Key"};
		const size_t nTagNames=4;
		for (size_t i = 0; i < nTagNames; i++) {
			if (node->getChild(string(tagNames[i])) == NULL) {
				//not all of necessary tags are defined in the node configration part
				return NULL;
			}
		}

		RMAPTargetNode* targetNode = new RMAPTargetNode();
		targetNode->setID(node->getAttribute("id"));
		targetNode->setTargetLogicalAddress(String::toInteger(node->getChild("TargetLogicalAddress")->getValue()));
		vector<unsigned char> targetSpaceWireAddress = String::toUnsignedCharArray(node->getChild(
						"TargetSpaceWireAddress")->getValue());
		targetNode->setTargetSpaceWireAddress(targetSpaceWireAddress);
		vector<unsigned char> replyAddress = String::toUnsignedCharArray(node->getChild("ReplyAddress")->getValue());
		targetNode->setReplyAddress(replyAddress);
		targetNode->setDefaultKey(String::toInteger(node->getChild("Key")->getValue()));

		//optional
		if (node->getChild("InitiatorLogicalAddress") != NULL) {
			using namespace std;
			targetNode->setInitiatorLogicalAddress(node->getChild("InitiatorLogicalAddress")->getValueAsUInt8());
		}
		constructRMAPMemoryObjectFromXMLFile(node,targetNode);

		return targetNode;
	}

private:
	static void constructRMAPMemoryObjectFromXMLFile(XMLNode* node,RMAPTargetNode* targetNode) throw (XMLLoader::XMLLoaderException,RMAPTargetNodeException,RMAPMemoryObjectException) {
		using namespace std;
		vector<XMLNode*> nodes = node->getChildren("RMAPMemoryObject");
		for (unsigned int i = 0; i < nodes.size(); i++) {
			if(nodes[i]!=NULL) {
				try {
					targetNode->addMemoryObject(RMAPMemoryObject::constructFromXMLNode(nodes[i]));
				} catch(RMAPMemoryObjectException e) {
					throw e;
				} catch(...) {
					throw RMAPMemoryObjectException(RMAPMemoryObjectException::InvalidXMLEntry);
				}
			} else {
				throw RMAPMemoryObjectException(RMAPMemoryObjectException::InvalidXMLEntry);
			}
		}
	}

#endif

public:
	uint8_t getDefaultKey() const {
		return defaultKey;
	}

	std::vector<uint8_t> getReplyAddress() const {
		return replyAddress;
	}

	uint8_t getTargetLogicalAddress() const {
		return targetLogicalAddress;
	}

	std::vector<uint8_t> getTargetSpaceWireAddress() const {
		return targetSpaceWireAddress;
	}

	void setDefaultKey(uint8_t defaultKey) {
		this->defaultKey = defaultKey;
	}

	void setReplyAddress(std::vector<uint8_t>& replyAddress) {
		this->replyAddress = replyAddress;
	}

	void setTargetLogicalAddress(uint8_t targetLogicalAddress) {
		this->targetLogicalAddress = targetLogicalAddress;
	}

	void setTargetSpaceWireAddress(std::vector<uint8_t>& targetSpaceWireAddress) {
		this->targetSpaceWireAddress = targetSpaceWireAddress;
	}

	void setInitiatorLogicalAddress(uint8_t initiatorLogicalAddress) {
		this->isInitiatorLogicalAddressSet_ = true;
		this->initiatorLogicalAddress = initiatorLogicalAddress;
	}

	void unsetInitiatorLogicalAddress() {
		this->isInitiatorLogicalAddressSet_ = false;
		this->initiatorLogicalAddress = 0xFE;
	}

	bool isInitiatorLogicalAddressSet() {
		return isInitiatorLogicalAddressSet_;
	}

	uint8_t getInitiatorLogicalAddress() {
		return initiatorLogicalAddress;
	}

public:
	void addMemoryObject(RMAPMemoryObject* memoryObject) {
		memoryObjects[memoryObject->getID()] = memoryObject;
	}

	std::map<std::string, RMAPMemoryObject*>* getMemoryObjects() {
		return &memoryObjects;
	}

	/** This method does not return NULL even when not found, but throws an exception. */
	RMAPMemoryObject* getMemoryObject(std::string memoryObjectID) throw (RMAPTargetNodeException) {
		std::map<std::string, RMAPMemoryObject*>::iterator it = memoryObjects.find(memoryObjectID);
		if (it != memoryObjects.end()) {
			return it->second;
		} else {
			throw RMAPTargetNodeException(RMAPTargetNodeException::NoSuchRMAPMemoryObject);
		}
	}

	/** This method can return NULL when not found.*/
	RMAPMemoryObject* findMemoryObject(std::string memoryObjectID) throw (RMAPTargetNodeException) {
		std::map<std::string, RMAPMemoryObject*>::iterator it = memoryObjects.find(memoryObjectID);
		if (it != memoryObjects.end()) {
			return it->second;
		} else {
			return NULL;
		}
	}
public:
	std::string toString() {
		using namespace std;
		stringstream ss;
		if (isInitiatorLogicalAddressSet()) {
			ss << "Initiator Logical Address : 0x" << right << hex << setw(2) << setfill('0')
					<< (uint32_t) initiatorLogicalAddress << endl;
		}
		ss << "Target Logical Address    : 0x" << right << hex << setw(2) << setfill('0')
				<< (uint32_t) targetLogicalAddress << endl;
		ss << "Target SpaceWire Address  : " << SpaceWireUtilities::packetToString(&targetSpaceWireAddress) << endl;
		ss << "Reply Address             : " << SpaceWireUtilities::packetToString(&replyAddress) << endl;
		ss << "Default Key               : 0x" << right << hex << setw(2) << setfill('0') << (uint32_t) defaultKey
				<< endl;
		std::map<std::string, RMAPMemoryObject*>::iterator it = memoryObjects.begin();
		for (; it != memoryObjects.end(); it++) {
			ss << it->second->toString();
		}
		return ss.str();
	}

	std::string toXMLString(int nTabs = 0) {
		using namespace std;
		stringstream ss;
		ss << "<RMAPTargetNode>" << endl;
		if (isInitiatorLogicalAddressSet()) {
			ss << "	<InitiatorLogicalAddress>" << "0x" << hex << right << setw(2) << setfill('0')
					<< (uint32_t) initiatorLogicalAddress << "</InitiatorLogicalAddress>" << endl;
		}
		ss << "	<TargetLogicalAddress>" << "0x" << hex << right << setw(2) << setfill('0')
				<< (uint32_t) targetLogicalAddress << "</TargetLogicalAddress>" << endl;
		ss << "	<TargetSpaceWireAddress>" << SpaceWireUtilities::packetToString(&targetSpaceWireAddress)
				<< "</TargetSpaceWireAddress>" << endl;
		ss << "	<ReplyAddress>" << SpaceWireUtilities::packetToString(&replyAddress) << "</ReplyAddress>" << endl;
		ss << "	<DefaultKey>" << "0x" << hex << right << setw(2) << setfill('0') << (uint32_t) defaultKey
				<< "</DefaultKey>" << endl;
		std::map<std::string, RMAPMemoryObject*>::iterator it = memoryObjects.begin();
		for (; it != memoryObjects.end(); it++) {
			ss << it->second->toXMLString(nTabs + 1);
		}
		ss << "</RMAPTargetNode>";

		stringstream ss2;
		while (!ss.eof()) {
			string line;
			getline(ss, line);
			for (int i = 0; i < nTabs; i++) {
				ss2 << "	";
			}
			ss2 << line << endl;
		}
		return ss2.str();
	}
};

class RMAPTargetNodeDBException : public CxxUtilities::Exception {
public:
	enum{
		NoSuchRMAPTargetNode
	};
public:
	RMAPTargetNodeDBException(int status) : CxxUtilities::Exception(status){

	}
};

class RMAPTargetNodeDB {
private:
	std::map<std::string, RMAPTargetNode*> db;

public:
	RMAPTargetNodeDB() {

	}

	RMAPTargetNodeDB(std::vector<RMAPTargetNode*> rmapTargetNodes) {
		addRMAPTargetNodes(rmapTargetNodes);
	}

	RMAPTargetNodeDB(std::string filename) {
		try {
			addRMAPTargetNodes(RMAPTargetNode::constructFromXMLFile(filename));
		} catch (...) {
			//empty DB will be constructed if the specified xml file contains any error.
		}
	}

	~RMAPTargetNodeDB() {
		//deletion of RMAPTargetNode* will be done outside this class.
	}

public:
	void addRMAPTargetNode(RMAPTargetNode* rmapTargetNode) {
		db[rmapTargetNode->getID()] = rmapTargetNode;
	}

	void addRMAPTargetNodes(std::vector<RMAPTargetNode*> rmapTargetNodes) {
		for (size_t i = 0; i < rmapTargetNodes.size(); i++) {
			addRMAPTargetNode(rmapTargetNodes[i]);
		}
	}

public:
	void loadRMAPTargetNodesFromXMLFile(std::string filename) throw (XMLLoader::XMLLoaderException,
			RMAPTargetNodeException, RMAPMemoryObjectException) {
		addRMAPTargetNodes(RMAPTargetNode::constructFromXMLFile(filename));
	}

public:
	/** This method does not return NULL when not found, but throws an exception.
	 *
	 */
	RMAPTargetNode* getRMAPTargetNode(std::string id) throw (RMAPTargetNodeDBException){
		std::map<std::string, RMAPTargetNode*>::iterator it = db.find(id);
		if (it != db.end()) {
			return it->second;
		} else {
			throw RMAPTargetNodeDBException(RMAPTargetNodeDBException::NoSuchRMAPTargetNode);
		}
	}

	/** This method can return NULL when an RMAPTargetNode with an specified ID is not found.
	 */
	RMAPTargetNode* findRMAPTargetNode(std::string id) {
		std::map<std::string, RMAPTargetNode*>::iterator it = db.find(id);
		if (it != db.end()) {
			return it->second;
		} else {
			return NULL;
		}
	}
};

#endif /* RMAPTARGETNODE_HH_ */