/*
 * RMAPInitiator.hh
 *
 *  Created on: Aug 2, 2011
 *      Author: yuasa
 */

#ifndef RMAPINITIATOR_HH_
#define RMAPINITIATOR_HH_

#include "RMAPPacket.hh"
#include "RMAPEngine.hh"
#include "RMAPTransaction.hh"
#include "RMAPTargetNode.hh"
#include "RMAPReplyStatus.hh"
#include "RMAPReplyException.hh"
#include "RMAPProtocol.hh"
#include "RMAPMemoryObject.hh"

class RMAPInitiatorException: public CxxUtilities::Exception {
public:
	enum {
		Timeout = 0x100,
		Aborted = 0x200,
		ReadReplyWithInsufficientData,
		ReadReplyWithTooMuchData,
		UnexpectedWriteReplyReceived,
		NoSuchRMAPMemoryObject,
		NoSuchRMAPTargetNode,
		RMAPTransactionCouldNotBeInitiated,
		SpecifiedRMAPMemoryObjectIsNotReadable,
		SpecifiedRMAPMemoryObjectIsNotWritable,
		SpecifiedRMAPMemoryObjectIsNotRMWable,
		RMAPTargetNodeDBIsNotRegistered
	};

public:
	RMAPInitiatorException(uint32_t status) :
		CxxUtilities::Exception(status) {
	}

public:
	std::string toString() {
		std::string result;
		switch (status) {
		case Timeout:
			result = "Timeout";
			break;
		case Aborted:
			result = "Aborted";
			break;
		case ReadReplyWithInsufficientData:
			result = "ReadReplyWithInsufficientData";
			break;
		case ReadReplyWithTooMuchData:
			result = "ReadReplyWithTooMuchData";
			break;
		case UnexpectedWriteReplyReceived:
			result = "UnexpectedWriteReplyReceived";
			break;
		case NoSuchRMAPMemoryObject:
			result = "NoSuchRMAPMemoryObject";
			break;
		case NoSuchRMAPTargetNode:
			result = "NoSuchRMAPTargetNode";
			break;
		case RMAPTransactionCouldNotBeInitiated:
			result = "RMAPTransactionCouldNotBeInitiated";
			break;
		case SpecifiedRMAPMemoryObjectIsNotReadable:
			result = "SpecifiedRMAPMemoryObjectIsNotReadable";
			break;
		case SpecifiedRMAPMemoryObjectIsNotWritable:
			result = "SpecifiedRMAPMemoryObjectIsNotWritable";
			break;
		case SpecifiedRMAPMemoryObjectIsNotRMWable:
			result = "SpecifiedRMAPMemoryObjectIsNotRMWable";
			break;
		case RMAPTargetNodeDBIsNotRegistered:
			result = "RMAPTargetNodeDBIsNotRegistered";
			break;
		default:
			result = "Undefined status";
			break;
		}
		return result;
	}
};

class RMAPInitiator {
private:
	RMAPEngine* rmapEngine;
	RMAPPacket* commandPacket;
	RMAPPacket* replyPacket;
	CxxUtilities::Mutex mutex;

private:
	//optional DB
	RMAPTargetNodeDB* targetNodeDB;

private:
	uint8_t initiatorLogicalAddress;
	bool isInitiatorLogicalAddressSet_;

private:
	bool incrementMode;
	bool isIncrementModeSet_;
	bool verifyMode;
	bool isVerifyModeSet_;
	bool replyMode;
	bool isReplyModeSet_;
	uint16_t transactionID;
	bool isTransactionIDSet_;

public:
	RMAPInitiator(RMAPEngine *rmapEngine) {
		this->rmapEngine = rmapEngine;
		commandPacket = new RMAPPacket();
		replyPacket = new RMAPPacket();
		isIncrementModeSet_ = false;
		isVerifyModeSet_ = false;
		isReplyModeSet_ = false;
		isTransactionIDSet_ = false;
	}

	~RMAPInitiator() {
		if(commandPacket!=NULL){
			delete commandPacket;
		}
		if (replyPacket != NULL) {
			deleteReplyPacket();
		}
	}

public:
	void deleteReplyPacket() {
		if(replyPacket==NULL){
			return;
		}
		delete replyPacket;
		replyPacket = NULL;
	}

public:
	void lock() {
		using namespace std;
		//		cout << "RMAPInitiator Locking a mutex." << endl;
		mutex.lock();
	}

	void unlock() {
		using namespace std;
		//		cout << "RMAPInitiator UnLocking a mutex." << endl;
		mutex.unlock();
	}

public:
	void read(std::string targetNodeID, uint32_t memoryAddress, uint32_t length, uint8_t* buffer,
			double timeoutDuration = DefaultTimeoutDuration) throw (RMAPEngineException, RMAPInitiatorException,
			RMAPReplyException) {
		if (targetNodeDB == NULL) {
			throw RMAPInitiatorException(RMAPInitiatorException::RMAPTargetNodeDBIsNotRegistered);
		}
		RMAPTargetNode* targetNode;
		try {
			targetNode = targetNodeDB->getRMAPTargetNode(targetNodeID);
		} catch (RMAPTargetNodeDBException e) {
			throw RMAPInitiatorException(RMAPInitiatorException::NoSuchRMAPTargetNode);
		}
		read(targetNode, memoryAddress, length, buffer, timeoutDuration);
	}

	/** easy to use, but somewhat slow due to data copy. */
	std::vector<uint8_t>* readConstructingNewVecotrBuffer(std::string targetNodeID, std::string memoryObjectID,
			double timeoutDuration = DefaultTimeoutDuration) throw (RMAPEngineException, RMAPInitiatorException,
			RMAPReplyException) {
		using namespace std;
		if (targetNodeDB == NULL) {
			throw RMAPInitiatorException(RMAPInitiatorException::RMAPTargetNodeDBIsNotRegistered);
		}
		cerr << targetNodeDB->getSize() << endl;
		RMAPTargetNode* targetNode;
		try {
			targetNode = targetNodeDB->getRMAPTargetNode(targetNodeID);
		} catch (RMAPTargetNodeDBException e) {
			throw RMAPInitiatorException(RMAPInitiatorException::NoSuchRMAPTargetNode);
		}

		RMAPMemoryObject* memoryObject;
		try {
			memoryObject = targetNode->getMemoryObject(memoryObjectID);
		} catch (RMAPTargetNodeException e) {
			throw RMAPInitiatorException(RMAPInitiatorException::NoSuchRMAPMemoryObject);
		}

		//check if the memory is readable.
		if (!memoryObject->isReadable()) {
			throw RMAPInitiatorException(RMAPInitiatorException::SpecifiedRMAPMemoryObjectIsNotReadable);
		}

		std::vector<uint8_t>* buffer = new std::vector<uint8_t>(memoryObject->getLength());
		read(targetNode, memoryObject->getAddress(), memoryObject->getLength(), &(buffer->at(0)), timeoutDuration);
		return buffer;

		return new std::vector<uint8_t>(4);
	}

	void read(std::string targetNodeID, std::string memoryObjectID, uint8_t* buffer, double timeoutDuration =
			DefaultTimeoutDuration) throw (RMAPEngineException, RMAPInitiatorException, RMAPReplyException) {
		if (targetNodeDB == NULL) {
			throw RMAPInitiatorException(RMAPInitiatorException::RMAPTargetNodeDBIsNotRegistered);
		}
		RMAPTargetNode* targetNode;
		try {
			targetNode = targetNodeDB->getRMAPTargetNode(targetNodeID);
		} catch (RMAPTargetNodeDBException e) {
			throw RMAPInitiatorException(RMAPInitiatorException::NoSuchRMAPMemoryObject);
		}
		read(targetNode, memoryObjectID, buffer, timeoutDuration);
	}

	void read(RMAPTargetNode* rmapTargetNode, std::string memoryObjectID, uint8_t *buffer, double timeoutDuration =
			DefaultTimeoutDuration) throw (RMAPEngineException, RMAPInitiatorException, RMAPReplyException) {
		RMAPMemoryObject* memoryObject;
		try {
			memoryObject = rmapTargetNode->getMemoryObject(memoryObjectID);
		} catch (RMAPTargetNodeException e) {
			throw RMAPInitiatorException(RMAPInitiatorException::NoSuchRMAPMemoryObject);
		}
		//check if the memory is readable.
		if (!memoryObject->isReadable()) {
			throw RMAPInitiatorException(RMAPInitiatorException::SpecifiedRMAPMemoryObjectIsNotReadable);
		}
		read(rmapTargetNode, memoryObject->getAddress(), memoryObject->getLength(), buffer, timeoutDuration);
	}

	void read(RMAPTargetNode* rmapTargetNode, uint32_t memoryAddress, uint32_t length, uint8_t *buffer,
			double timeoutDuration = DefaultTimeoutDuration) throw (RMAPEngineException, RMAPInitiatorException,
			RMAPReplyException) {
		using namespace std;
		lock();
		if (replyPacket != NULL) {
			deleteReplyPacket();
		}
		commandPacket->setInitiatorLogicalAddress(this->getInitiatorLogicalAddress());
		commandPacket->setRead();
		commandPacket->setCommand();
		if (incrementMode) {
			commandPacket->setIncrementMode();
		} else {
			commandPacket->setNoIncrementMode();
		}
		commandPacket->setNoVerifyMode();
		commandPacket->setReplyMode();
		commandPacket->setExtendedAddress(0x00);
		commandPacket->setAddress(memoryAddress);
		commandPacket->setDataLength(length);
		commandPacket->clearData();
		/** InitiatorLogicalAddress might be updated in commandPacket->setRMAPTargetInformation(rmapTargetNode) below */
		commandPacket->setRMAPTargetInformation(rmapTargetNode);
		RMAPTransaction transaction;
		transaction.commandPacket = this->commandPacket;
		//tid
		if (isTransactionIDSet_) {
			transaction.setTransactionID(transactionID);
		}
		try {
			rmapEngine->initiateTransaction(transaction);
		} catch (RMAPEngineException e) {
			unlock();
			throw RMAPInitiatorException(RMAPInitiatorException::RMAPTransactionCouldNotBeInitiated);
		} catch (...) {
			unlock();
			throw RMAPInitiatorException(RMAPInitiatorException::RMAPTransactionCouldNotBeInitiated);
		}
		transaction.condition.wait(timeoutDuration);
		if (transaction.state == RMAPTransaction::ReplyReceived) {
			replyPacket = transaction.replyPacket;
			if (replyPacket->getStatus() != RMAPReplyStatus::CommandExcecutedSuccessfully) {
				uint8_t replyStatus=replyPacket->getStatus();
				unlock();
				deleteReplyPacket();
				throw RMAPReplyException(replyStatus);
			}
			if (replyPacket->getDataBuffer()->size() != length) {
				unlock();
				deleteReplyPacket();
				throw RMAPInitiatorException(RMAPInitiatorException::ReadReplyWithInsufficientData);
			}
			replyPacket->getData(buffer, length);
			unlock();
			//when successful, replay packet is retained until next transaction for inspection by user application
			//deleteReplyPacket();
			return;
		} else {
			transaction.state = RMAPTransaction::Timeout;
			//cancel transaction (return transaction ID)
			rmapEngine->cancelTransaction(&transaction);
			unlock();
			deleteReplyPacket();
			throw RMAPInitiatorException(RMAPInitiatorException::Timeout);
		}
	}

public:
	void write(std::string targetNodeID, uint32_t memoryAddress, uint8_t *data, uint32_t length,
			double timeoutDuration = DefaultTimeoutDuration) throw (RMAPEngineException, RMAPInitiatorException,
			RMAPReplyException) {
		if (targetNodeDB == NULL) {
			throw RMAPInitiatorException(RMAPInitiatorException::RMAPTargetNodeDBIsNotRegistered);
		}
		RMAPTargetNode *targetNode;
		try {
			targetNode = targetNodeDB->getRMAPTargetNode(targetNodeID);
		} catch (RMAPTargetNodeDBException e) {
			throw RMAPInitiatorException(RMAPInitiatorException::NoSuchRMAPMemoryObject);
		}
		write(targetNode, memoryAddress, data, length, timeoutDuration);
	}

	void write(std::string targetNodeID, std::string memoryObjectID, uint8_t* data, double timeoutDuration =
			DefaultTimeoutDuration) throw (RMAPEngineException, RMAPInitiatorException, RMAPReplyException) {
		if (targetNodeDB == NULL) {
			throw RMAPInitiatorException(RMAPInitiatorException::RMAPTargetNodeDBIsNotRegistered);
		}
		RMAPTargetNode *targetNode;
		try {
			targetNode = targetNodeDB->getRMAPTargetNode(targetNodeID);
		} catch (RMAPTargetNodeDBException e) {
			throw RMAPInitiatorException(RMAPInitiatorException::NoSuchRMAPMemoryObject);
		}
		write(targetNode, memoryObjectID, data, timeoutDuration);
	}

	void write(RMAPTargetNode *rmapTargetNode, std::string memoryObjectID, uint8_t* data, double timeoutDuration =
			DefaultTimeoutDuration) throw (RMAPEngineException, RMAPInitiatorException, RMAPReplyException) {
		RMAPMemoryObject* memoryObject;
		try {
			memoryObject = rmapTargetNode->getMemoryObject(memoryObjectID);
		} catch (RMAPTargetNodeException e) {
			throw RMAPInitiatorException(RMAPInitiatorException::NoSuchRMAPMemoryObject);
		}
		//check if the memory is readable.
		if (!memoryObject->isWritable()) {
			throw RMAPInitiatorException(RMAPInitiatorException::SpecifiedRMAPMemoryObjectIsNotWritable);
		}
		write(rmapTargetNode, memoryObject->getAddress(), data, memoryObject->getLength(), timeoutDuration);
	}

	void write(RMAPTargetNode *rmapTargetNode, uint32_t memoryAddress, std::vector<uint8_t>* data,
			double timeoutDuration = DefaultTimeoutDuration) throw (RMAPEngineException, RMAPInitiatorException,
			RMAPReplyException) {
		uint8_t* pointer=NULL;
		if(data->size()!=0){
			pointer=&(data->at(0));
		}
		write(rmapTargetNode,memoryAddress,pointer,data->size(),timeoutDuration);
	}

	void write(RMAPTargetNode *rmapTargetNode, uint32_t memoryAddress, uint8_t *data, uint32_t length,
			double timeoutDuration = DefaultTimeoutDuration) throw (RMAPEngineException, RMAPInitiatorException,
			RMAPReplyException) {
		lock();
		if (replyPacket != NULL) {
			deleteReplyPacket();
		}
		commandPacket->setInitiatorLogicalAddress(this->getInitiatorLogicalAddress());
		commandPacket->setWrite();
		commandPacket->setCommand();
		if (incrementMode) {
			commandPacket->setIncrementMode();
		} else {
			commandPacket->setNoIncrementMode();
		}
		if (verifyMode) {
			commandPacket->setVerifyMode();
		} else {
			commandPacket->setNoVerifyMode();
		}
		if (replyMode) {
			commandPacket->setReplyMode();
		} else {
			commandPacket->setNoReplyMode();
		}
		commandPacket->setExtendedAddress(0x00);
		commandPacket->setAddress(memoryAddress);
		commandPacket->setDataLength(length);
		commandPacket->setRMAPTargetInformation(rmapTargetNode);
		commandPacket->setData(data, length);
		RMAPTransaction transaction;
		transaction.commandPacket = this->commandPacket;
		setRMAPTransactionOptions(transaction);
		rmapEngine->initiateTransaction(transaction);

		if (!replyMode) {//if reply is not expected
			if (transaction.state == RMAPTransaction::Initiated) {
				unlock();
				return;
			} else {
				unlock();
				//command was not sent successfully
				throw RMAPInitiatorException(RMAPInitiatorException::RMAPTransactionCouldNotBeInitiated);
			}
		}
		transaction.state=RMAPTransaction::CommandSent;

		//if reply is expected
		transaction.condition.wait(timeoutDuration);
		replyPacket = transaction.replyPacket;
		if (transaction.state == RMAPTransaction::CommandSent) {
			if (replyMode) {
				unlock();
				throw RMAPInitiatorException(RMAPInitiatorException::Timeout);
			} else {
				unlock();
				return;
			}
		} else if (transaction.state == RMAPTransaction::ReplyReceived) {
			replyPacket = transaction.replyPacket;
			if (replyPacket->getStatus() != RMAPReplyStatus::CommandExcecutedSuccessfully) {
				uint8_t replyStatus=replyPacket->getStatus();
				unlock();
				deleteReplyPacket();
				throw RMAPReplyException(replyStatus);
			}
			if (replyPacket->getStatus() == RMAPReplyStatus::CommandExcecutedSuccessfully) {
				unlock();
				//When successful, replay packet is retained until next transaction for inspection by user application
				//deleteReplyPacket();
				return;
			} else {
				uint8_t replyStatus=replyPacket->getStatus();
				unlock();
				deleteReplyPacket();
				throw RMAPReplyException(replyStatus);
			}
		} else if (transaction.state == RMAPTransaction::Timeout) {
			unlock();
			//cancel transaction (return transaction ID)
			rmapEngine->cancelTransaction(&transaction);
			deleteReplyPacket();
			throw RMAPInitiatorException(RMAPInitiatorException::Timeout);
		}
	}

private:
	void setRMAPTransactionOptions(RMAPTransaction& transaction) {
		//increment mode
		if (isIncrementModeSet_) {
			if (incrementMode) {
				transaction.commandPacket->setIncrementFlag();
			} else {
				transaction.commandPacket->unsetIncrementFlag();
			}
		}
		//verify mode
		if (isVerifyModeSet_) {
			if (verifyMode) {
				transaction.commandPacket->setVerifyFlag();
			} else {
				transaction.commandPacket->setNoVerifyMode();
			}
		}
		//reply mode
		if (transaction.commandPacket->isRead() && transaction.commandPacket->isCommand()) {
			transaction.commandPacket->setReplyMode();
		} else if (isReplyModeSet_) {
			if (replyMode) {
				transaction.commandPacket->setReplyMode();
			} else {
				transaction.commandPacket->setNoReplyMode();
			}
		}
		//tid
		if (isTransactionIDSet_) {
			transaction.setTransactionID(transactionID);
		}
	}

public:
	void setInitiatorLogicalAddress(uint8_t initiatorLogicalAddress) {
		this->initiatorLogicalAddress = initiatorLogicalAddress;
		isInitiatorLogicalAddressSet_=true;
	}

	uint8_t getInitiatorLogicalAddress() {
		if(isInitiatorLogicalAddressSet_==true){
			return initiatorLogicalAddress;
		}else{
			return RMAPProtocol::DefaultLogicalAddress;
		}
	}

	bool isInitiatorLogicalAddressSet(){
		return isInitiatorLogicalAddressSet_;
	}

	void unsetInitiatorLogicalAddress(){
		isInitiatorLogicalAddressSet_=false;
	}

public:
	static const double DefaultTimeoutDuration = 1000.0;

public:
	bool getReplyMode() const {
		return replyMode;
	}

	void setReplyMode(bool replyMode) {
		this->replyMode = replyMode;
		this->isReplyModeSet_ = true;
	}

	void unsetReplyMode() {
		isReplyModeSet_ = false;
	}

	bool isReplyModeSet() {
		return isReplyModeSet_;
	}

public:
	bool getIncrementMode() const {
		return incrementMode;
	}

	void setIncrementMode(bool incrementMode) {
		this->incrementMode = incrementMode;
		this->isIncrementModeSet_ = true;
	}

	void unsetIncrementMode() {
		this->isIncrementModeSet_ = false;
	}

	bool isIncrementModeSet() {
		return isIncrementModeSet_;
	}

public:
	bool getVerifyMode() const {
		return verifyMode;
	}

	void setVerifyMode(bool verifyMode) {
		this->verifyMode = verifyMode;
		this->isVerifyModeSet_ = true;
	}

	void unsetVerifyMode() {
		this->isVerifyModeSet_ = false;
	}

	bool isVerifyModeSet() {
		return isVerifyModeSet_;
	}

public:
	void setTransactionID(uint16_t transactionID) {
		this->transactionID = transactionID;
		this->isTransactionIDSet_ = true;
	}

	void unsetTransactionID() {
		this->isTransactionIDSet_ = false;
	}

	uint16_t getTransactionID() {
		return transactionID;
	}

	bool isTransactionIDSet() {
		return isTransactionIDSet_;
	}

public:
	RMAPPacket* getCommandPacketPointer() {
		return commandPacket;
	}

	RMAPPacket* getReplyPacketPointer() {
		return replyPacket;
	}

public:
	void setRMAPTargetNodeDB(RMAPTargetNodeDB* targetNodeDB) {
		this->targetNodeDB = targetNodeDB;
	}

	RMAPTargetNodeDB* getRMAPTargetNodeDB() {
		return targetNodeDB;
	}

};
#endif /* RMAPINITIATOR_HH_ */
