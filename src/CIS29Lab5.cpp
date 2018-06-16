//============================================================================
// Name        : Lab4
// Author      : Branden Lee
// Date        : 5/30/2018
// Description : Encryption and Compression
//============================================================================

/**
 * Objectives:
 * 1. No raw pointers
 * 	In previous labs I used raw pointers frequently. My labs assume the compiler is "dumb"
 * 	and does not perform optimizations very well, such as RVO (Return Value Optimization).
 * 	In this lab I have attempted to use smart pointers and references. I still do not return objects
 * 	by value even though any modern compiler should optimize this very well.
 */

#include <string>
#include <fstream>
#include <iostream>			// std::cout
#include <sstream>
#include <iomanip>
#include <vector>			// std::vector
#include <stack>
#include <queue>			// std::priority_queue
#include <cstring>
#include <memory>			// std::unique_ptr
#include <functional>		// std::hash
#include <bitset>			// std::bitset
#include <stdarg.h>			// std::va_list
#include <unordered_map>	// std::unordered_map
using namespace std;

/** Buffer size for reading in files for parsing */
#define STREAM_SCANNER_BUFFER_SIZE 4096

/**
 @class FileHandler
 simply reads or writes the decoded or encoded message to a file\n
 */
class FileHandler {
public:
	FileHandler() {
	}
	~FileHandler() {
	}
	/** takes a file stream by reference and opens a file.\n
	 * the reason we do not return the string of the entire ASCII file
	 * is because we want to stream and not waste memory
	 @pre None
	 @post None
	 @param string File name to open
	 @return True on file open successful and false in not
	 */
	bool readStream(string fileName, ifstream& fileStream);
	bool writeStream(string fileName, ofstream& fileStream);
	bool writeString(string fileName, string stringValue);
	bool close(ifstream& fileStream);
	bool close(ofstream& fileStream);
};

/**
 @class CharacterFrequencyNode
 similar to std::pair<unsigned int, unsigned int> except has that operator< overloaded \n
 first = frequency \n
 second = ascii character code \n
 */
class CharacterFrequencyNode {
public:
	unsigned int frequency, characterCode;
	CharacterFrequencyNode(unsigned int frequency_,
			unsigned int characterCode_);
	// "this" is already the left hand side
	bool operator<(const CharacterFrequencyNode &rhs) {
		return frequency < rhs.frequency;
	}
	bool operator>(const CharacterFrequencyNode &rhs) {
		return frequency > rhs.frequency;
	}
};

/**
 @class CharacterPriorityQueueTreeNode
 */
class CharacterPriorityQueueTreeNode {
private:
public:
	unsigned int frequency;
	CharacterPriorityQueueTreeNode() :
			frequency(0) {

	}
	virtual ~CharacterPriorityQueueTreeNode() {

	}
	unsigned int getFrequency();
	virtual bool isLeaf() {
		return false;
	}
	virtual bool isBranch() {
		return false;
	}
};

/**
 @class CharacterPriorityQueueTreeLeaf
 */
class CharacterPriorityQueueTreeLeaf: public CharacterPriorityQueueTreeNode {
private:
	shared_ptr<CharacterFrequencyNode> characterFrequencyNode;
public:
	CharacterPriorityQueueTreeLeaf(
			shared_ptr<CharacterFrequencyNode> characterNode) :
			characterFrequencyNode(characterNode) {
		frequency = characterNode->frequency;
	}
	~CharacterPriorityQueueTreeLeaf() {

	}
	bool isLeaf();
	bool isBranch();
	shared_ptr<CharacterFrequencyNode> getCharacterNode();
};

/**
 @class CharacterPriorityQueueTreeBranch
 */
class CharacterPriorityQueueTreeBranch: public CharacterPriorityQueueTreeNode {
private:
	shared_ptr<CharacterPriorityQueueTreeNode> left, right;
public:
	CharacterPriorityQueueTreeBranch(
			shared_ptr<CharacterPriorityQueueTreeNode> left_,
			shared_ptr<CharacterPriorityQueueTreeNode> right_) :
			left(left_), right(right_) {
		frequency = left_->getFrequency() + right_->getFrequency();
	}
	~CharacterPriorityQueueTreeBranch() {

	}
	bool isLeaf();
	bool isBranch();
	shared_ptr<CharacterPriorityQueueTreeNode> getLeft();
	shared_ptr<CharacterPriorityQueueTreeNode> getRight();
};

/**
 * @class CharacterPriorityQueueCompare
 * Function object for performing comparisons. Uses operator< on type T. \n
 */
template<class T>
class CharacterPriorityQueueCompare {
public:
	bool operator()(const T &lhs, const T &rhs) const {
		return lhs->getFrequency() > rhs->getFrequency();
	}
};

/**
 * @class StreamScanner
 * A reusable class to scan and parse a stream \n
 */
class StreamScanner {
public:
	StreamScanner() {

	}
	virtual ~StreamScanner() {

	}
	bool scanStream(istream& streamIn, ...);
	virtual bool bufferHandle(string& streamBuffer, bool final, va_list& args) {
		return false;
	}
};

/**
 * @class FileData
 * holds some data about the original file for the decompressor \n
 */
class FileData {
private:
	unsigned int fileSize;
public:
	FileData() :
			fileSize(0) {

	}
	~FileData() {
	}
	bool stream(istream& streamIn);
	unsigned int getFileSize();
};

/**
 * The priority queue type is long so we make it into an alias
 * using is used rather than typedef because of this forum:
 * https://stackoverflow.com/questions/10747810/what-is-the-difference-between-typedef-and-using-in-c11
 */
using priorityQueueType = std::priority_queue<shared_ptr<CharacterPriorityQueueTreeNode>, vector<shared_ptr<CharacterPriorityQueueTreeNode>>, CharacterPriorityQueueCompare<shared_ptr<CharacterPriorityQueueTreeNode>>>;

/**
 * @class CharacterPriorityQueue
 * document -> unordered_map -> priority_queue -> set -> binary string \n
 */
class CharacterPriorityQueue: public StreamScanner {
private:
	/** unordered_map<character code, character frequency>
	 * character frequency is a shared pointer to make it easy to increment */
	unordered_map<unsigned int, shared_ptr<unsigned int>> characterFrequencyTable;
	priorityQueueType priorityQueue;
public:
	CharacterPriorityQueue() {
		// enough for ascii characters
		characterFrequencyTable.reserve(255);
	}
	~CharacterPriorityQueue() {
	}
	/**
	 * parses document into a character frequency table \n
	 * Afterward, call <pre>buildPriorityQueue()</pre> to build the priority queue
	 * @return true on parse success, false on failure
	 */
	bool stream(istream& streamIn);
	bool bufferHandle(string& streamBuffer, bool final, va_list& args);
	/**
	 * builds the priority queue from the character frequency table
	 * @return true on build success, false on failure
	 */
	bool buildPriorityQueue();
	/**
	 * @return priority queue reference
	 */
	reference_wrapper<priorityQueueType> getPriorityQueue();
};

/**
 @class CharacterPriorityQueueTree
 Tables to convert character codes to binary strings and back \n
 */
class CharacterPriorityQueueTree {
private:
	shared_ptr<CharacterPriorityQueueTreeNode> characterPriorityQueueTreePtr;
	/** unordered_map<unsigned int, string> */
	shared_ptr<unordered_map<unsigned int, string>> characterToBinaryTablePtr;
public:
	CharacterPriorityQueueTree() {

	}
	~CharacterPriorityQueueTree() {

	}
	/**
	 * build tree will edit the priority queue so we must pass by value
	 * to prevent tampering
	 * @param priority queue copy
	 * @return true on build success, false on build failure
	 */
	bool buildTree(priorityQueueType priorityQueue);
	bool buildBinaryTable();
	void buildBinaryTableEncode(shared_ptr<CharacterPriorityQueueTreeNode> node,
			string binaryString);
	shared_ptr<unordered_map<unsigned int, string>> getCharacterToBinaryTable();
};

/**
 @class CharacterToBinaryTable
 Tables to convert character codes to binary strings and back \n
 */
class CharacterToBinaryTable {
private:
	/** unordered_map<character code, character binary string> */
	shared_ptr<unordered_map<unsigned int, string>> characterCodeToBinaryStringTablePtr;
	/** unordered_map<character binary string, character code> */
	shared_ptr<unordered_map<string, unsigned int>> binaryStringToCharacterCodeTablePtr;
	CharacterPriorityQueueTreeNode characterPriorityQueueTreeNodeParent;
public:
	CharacterToBinaryTable() {

	}
	/**
	 * to keep this class "slim", we will build our table elsewhere and move the pointer
	 * ownership into this class.
	 */
	void set(shared_ptr<unordered_map<unsigned int, string>> tablePtr);
	void set(shared_ptr<unordered_map<string, unsigned int>> tablePtr);
	/**
	 * build the binaryStringToCharacterCodeTable from the characterCodeToBinaryStringTable
	 */
	void buildBinaryStringToCharacterCodeTable();
	/**
	 * build the characterCodeToBinaryStringTable from the binaryStringToCharacterCodeTable
	 */
	void buildCharacterCodeToBinaryStringTable();
	/**
	 * Primary encoding method
	 * @param characterCode Character character code
	 * @return Character Binary String
	 */
	bool characterCodeToBinaryString(unsigned int characterCode,
			string& binaryString);
	/**
	 * Primary decoding method
	 * @param characterBinaryString Character Binary String
	 * @return character code
	 */
	bool binaryStringToCharacterCode(string binaryString,
			unsigned int& characterCode);
};

class Compressor: public StreamScanner {
private:
	shared_ptr<CharacterToBinaryTable> characterToBinaryTablePtr;
	string streamBufferOut;
public:
	Compressor() {

	}
	~Compressor() {

	}
	void set(shared_ptr<CharacterToBinaryTable> tablePtr);
	bool stream(istream& streamIn, ostream& streamOut);
	bool bufferHandle(string& streamBuffer, bool final, va_list& args);
	bool bufferOutHandle(string& streamBufferOut, bool final,
			ostream& streamOut);
};

class Extractor: public StreamScanner {
private:
	shared_ptr<CharacterToBinaryTable> characterToBinaryTablePtr;
	shared_ptr<FileData> fileDataPtr;
	string binaryString, streamBufferOut;
	unsigned int fileSizeCurrent, fileSizeTarget;
public:
	Extractor() :
			fileSizeCurrent(0), fileSizeTarget(0) {

	}
	~Extractor() {

	}
	void set(shared_ptr<CharacterToBinaryTable> tablePtr);
	void set(shared_ptr<FileData> fileDataPtr_);
	bool stream(istream& streamIn, ostream& streamOut);
	bool bufferHandle(string& streamBuffer, bool final, va_list& args);
	bool bufferBinaryHandle(bool final, ostream& streamOut);
	bool bufferOutHandle(bool final, ostream& streamOut);
};

/*
 * FileHandler Implementation
 */
bool FileHandler::readStream(string fileName, ifstream& fileStream) {
	fileStream.open(fileName, ios::binary);
	if (fileStream.is_open()) {
		return true;
	}
	throw 1;
	return false;
}

bool FileHandler::writeStream(string fileName, ofstream& fileStream) {
	fileStream.open(fileName, ios::binary);
	if (fileStream.is_open()) {
		return true;
	}
	throw 2;
	return false;
}

bool FileHandler::writeString(string fileName, string stringValue) {
	ofstream fileStream;
	fileStream.open(fileName);
	if (fileStream.is_open()) {
		fileStream << stringValue;
		fileStream.close();
		return true;
	}
	return false;
}

bool FileHandler::close(ifstream& fileStream) {
	try {
		fileStream.close();
	} catch (...) {
		throw 7;
		return false;
	}
	return true;
}

bool FileHandler::close(ofstream& fileStream) {
	try {
		fileStream.close();
	} catch (...) {
		throw 8;
		return false;
	}
	return true;
}

/*
 * CharacterFrequencyNode Implementation
 */
CharacterFrequencyNode::CharacterFrequencyNode(unsigned int frequency_,
		unsigned int characterCode_) {
	frequency = frequency_;
	characterCode = characterCode_;
}

/*
 * CharacterPriorityQueueTreeNode Implementation
 */
unsigned int CharacterPriorityQueueTreeNode::getFrequency() {
	return frequency;
}
/*
 * CharacterPriorityQueueTreeLeaf Implementation
 */
bool CharacterPriorityQueueTreeLeaf::isLeaf() {
	return true;
}
bool CharacterPriorityQueueTreeLeaf::isBranch() {
	return false;
}
shared_ptr<CharacterFrequencyNode> CharacterPriorityQueueTreeLeaf::getCharacterNode() {
	return characterFrequencyNode;
}

/*
 * CharacterPriorityQueueTreeBranch Implementation
 */
bool CharacterPriorityQueueTreeBranch::isLeaf() {
	return false;
}
bool CharacterPriorityQueueTreeBranch::isBranch() {
	return true;
}
shared_ptr<CharacterPriorityQueueTreeNode> CharacterPriorityQueueTreeBranch::getLeft() {
	return left;
}
shared_ptr<CharacterPriorityQueueTreeNode> CharacterPriorityQueueTreeBranch::getRight() {
	return right;
}
/*
 * CharacterPriorityQueue Implementation
 */
bool StreamScanner::scanStream(istream& streamIn, ...) {
	unsigned int fileSize, filePos, bufferSize, mode;
	string streamBuffer;
	stack<string> documentStack;
	bufferSize = STREAM_SCANNER_BUFFER_SIZE;
	fileSize = filePos = mode = 0;
	streamBuffer = "";
	char bufferInChar[STREAM_SCANNER_BUFFER_SIZE];
	streamIn.seekg(0, ios::end); // set the pointer to the end
	fileSize = static_cast<unsigned int>(streamIn.tellg()); // get the length of the file
	streamIn.seekg(0, ios::beg); // set the pointer to the beginning
	while (filePos < fileSize) {
		streamIn.seekg(filePos, ios::beg); // seek new position
		if (filePos + bufferSize > fileSize) {
			bufferSize = fileSize - filePos;
		}
		memset(bufferInChar, 0, sizeof(bufferInChar)); // zero out buffer
		streamIn.read(bufferInChar, bufferSize);
		streamBuffer.append(bufferInChar, bufferSize);
		va_list args;
		va_start(args, &streamIn);
		bufferHandle(streamBuffer, false, args);
		va_end(args);
		// advance buffer
		filePos += bufferSize;
	}
	// handle the remaining buffer
	va_list args;
	va_start(args, &streamIn);
	bufferHandle(streamBuffer, true, args);
	va_end(args);
	return true;
}
/*
 * FileData Implementation
 */
bool FileData::stream(istream& streamIn) {
	streamIn.seekg(0, ios::end); // set the pointer to the end
	fileSize = static_cast<unsigned int>(streamIn.tellg()); // get the length of the file
	streamIn.seekg(0, ios::beg); // set the pointer to the beginning
	return true;
}
unsigned int FileData::getFileSize() {
	return fileSize;
}
/*
 * CharacterPriorityQueue Implementation
 */
bool CharacterPriorityQueue::stream(istream& streamIn) {
	/* Parsing Steps:
	 * 1. read in buffer
	 * 2. go through each character in buffer
	 *    add the character code as hash table key and value as the frequency
	 * */
	return scanStream(streamIn);
}

bool CharacterPriorityQueue::bufferHandle(string& streamBuffer, bool final,
		va_list& args) {
	/* characters are placed into a hash table
	 * unordered_map[CharacterCode] = CharacterFrequency
	 */
	size_t i, n;
	n = streamBuffer.size();
	unsigned int characterCode;
	shared_ptr<unsigned int> characterFrequencyPtr;
	for (i = 0; i < n; i++) {
		characterCode =
				static_cast<unsigned int>(static_cast<int>(streamBuffer[i]));
		try {
			characterFrequencyPtr = characterFrequencyTable.at(characterCode);
			// character exists. increment frequency
			(*characterFrequencyPtr)++;
		} catch (...) {
			characterFrequencyTable.insert(
					{ characterCode, make_shared<unsigned int>(1) });
		}
	}
	streamBuffer = "";
	// there actually is no point to the return, but maybe there will be in the future
	return true;
}

bool CharacterPriorityQueue::buildPriorityQueue() {
	for (const auto& pair : characterFrequencyTable) {
		priorityQueue.push(
				dynamic_pointer_cast<CharacterPriorityQueueTreeNode>(
						make_shared<CharacterPriorityQueueTreeLeaf>(
								make_shared<CharacterFrequencyNode>(
										*(pair.second), pair.first))));
	}
	return true;
}

reference_wrapper<priorityQueueType> CharacterPriorityQueue::getPriorityQueue() {
	return ref(priorityQueue);
}

/*
 * CharacterPriorityQueueTree Implementation
 */
bool CharacterPriorityQueueTree::buildTree(priorityQueueType priorityQueue) {
	if (priorityQueue.size() > 0) {
		shared_ptr<CharacterPriorityQueueTreeNode> left;
		shared_ptr<CharacterPriorityQueueTreeNode> right;
		shared_ptr<CharacterPriorityQueueTreeNode> branch;
		while (priorityQueue.size() > 1) {
			left = priorityQueue.top();
			priorityQueue.pop();
			right = priorityQueue.top();
			priorityQueue.pop();
			branch = dynamic_pointer_cast<CharacterPriorityQueueTreeNode>(
					make_shared<CharacterPriorityQueueTreeBranch>(left, right));
			priorityQueue.push(branch);
		}
		characterPriorityQueueTreePtr = priorityQueue.top();
	}
	return true;
}
bool CharacterPriorityQueueTree::buildBinaryTable() {
	if (!characterToBinaryTablePtr) {
		characterToBinaryTablePtr = make_shared<
				unordered_map<unsigned int, string>>(255);
	}
	if (characterPriorityQueueTreePtr) {
		buildBinaryTableEncode(characterPriorityQueueTreePtr, "");
	}
	return true;
}
void CharacterPriorityQueueTree::buildBinaryTableEncode(
		shared_ptr<CharacterPriorityQueueTreeNode> node, string binaryString) {
	if (node->isBranch()) {
		if (dynamic_pointer_cast<CharacterPriorityQueueTreeBranch>(node)) {
			// copy each side before appending. append will modify original.
			string binaryStringLeft = binaryString;
			string binaryStringRight = binaryString;
			buildBinaryTableEncode(
					dynamic_pointer_cast<CharacterPriorityQueueTreeBranch>(node)->getLeft(),
					binaryStringLeft.append("0"));
			buildBinaryTableEncode(
					dynamic_pointer_cast<CharacterPriorityQueueTreeBranch>(node)->getRight(),
					binaryStringRight.append("1"));
		}
	} else {
		if (dynamic_pointer_cast<CharacterPriorityQueueTreeLeaf>(node)) {
			characterToBinaryTablePtr->insert(
					{
							dynamic_pointer_cast<CharacterPriorityQueueTreeLeaf>(
									node)->getCharacterNode()->characterCode,
							binaryString });
		}
	}
}
shared_ptr<unordered_map<unsigned int, string>> CharacterPriorityQueueTree::getCharacterToBinaryTable() {
	return characterToBinaryTablePtr;
}

/*
 * CharacterToBinaryTable Implementation
 */
void CharacterToBinaryTable::set(
		shared_ptr<unordered_map<unsigned int, string>> tablePtr) {
	characterCodeToBinaryStringTablePtr = tablePtr;
}

void CharacterToBinaryTable::set(
		shared_ptr<unordered_map<string, unsigned int>> tablePtr) {
	binaryStringToCharacterCodeTablePtr = tablePtr;
}

void CharacterToBinaryTable::buildBinaryStringToCharacterCodeTable() {
	/*
	 * builds unordered_map<binary string, character code>
	 */
	if (!binaryStringToCharacterCodeTablePtr) {
		// we only expect around 255 unique characters
		binaryStringToCharacterCodeTablePtr = make_shared<
				unordered_map<string, unsigned int>>(255);
	}
	for (const auto& pair : *characterCodeToBinaryStringTablePtr) {
		binaryStringToCharacterCodeTablePtr->insert(
				{ pair.second, pair.first });
	}
}

void CharacterToBinaryTable::buildCharacterCodeToBinaryStringTable() {
	/*
	 * builds unordered_map<character code, binary string>
	 */
	if (!characterCodeToBinaryStringTablePtr) {
		// we only expect around 255 unique characters
		characterCodeToBinaryStringTablePtr = make_shared<
				unordered_map<unsigned int, string>>(255);
	}
	for (const auto& pair : *binaryStringToCharacterCodeTablePtr) {
		characterCodeToBinaryStringTablePtr->insert(
				{ pair.second, pair.first });
	}
}

bool CharacterToBinaryTable::characterCodeToBinaryString(
		unsigned int characterCode, string& binaryString) {
	bool returnValue = false;
	try {
		binaryString = characterCodeToBinaryStringTablePtr->at(characterCode);
		returnValue = true;
	} catch (...) {
		// nothing
	}
	return returnValue;
}
bool CharacterToBinaryTable::binaryStringToCharacterCode(string binaryString,
		unsigned int& characterCode) {
	bool returnValue = false;
	try {
		characterCode = binaryStringToCharacterCodeTablePtr->at(binaryString);
		returnValue = true;
	} catch (...) {
		// nothing
	}
	return returnValue;
}

/*
 * Compressor Implementation
 */
void Compressor::set(shared_ptr<CharacterToBinaryTable> tablePtr) {
	characterToBinaryTablePtr = tablePtr;
}

bool Compressor::stream(istream& streamIn, ostream& streamOut) {
	/* Parsing Steps:
	 * 1. read in buffer
	 * 2. go through each character in buffer
	 *    add the character code as hash table key and value as the frequency
	 * */
	return scanStream(streamIn, &streamOut);
}
bool Compressor::bufferHandle(string& streamBuffer, bool final, va_list& args) {
	/* characters are placed into a hash table
	 * unordered_map[CharacterCode] = CharacterFrequency
	 */
	ostream& streamOut = va_arg(args, ostream);
	size_t i, n;
	n = streamBuffer.size();
	unsigned int characterCode;
	string binaryString;
	for (i = 0; i < n; i++) {
		characterCode =
				static_cast<unsigned int>(static_cast<int>(streamBuffer[i]));
		if (characterToBinaryTablePtr->characterCodeToBinaryString(
				characterCode, binaryString)) {
			streamBufferOut.append(binaryString);
			if (streamBufferOut.size() >= STREAM_SCANNER_BUFFER_SIZE) {
				bufferOutHandle(streamBufferOut, false, streamOut);
			}
		}
	}
	streamBuffer.clear();
	if (final) {
		bufferOutHandle(streamBufferOut, true, streamOut);
	}
	return true;
}

bool Compressor::bufferOutHandle(string& streamBufferOut, bool final,
		ostream& streamOut) {
	size_t i, n, r;
	if (final) {
		r = streamBufferOut.size() % 8;
		// fill end with zeros to be divisible by 8
		streamBufferOut.append(8 - r, '1');
	}
	n = streamBufferOut.size() / 8;
	char bufferOutChar[STREAM_SCANNER_BUFFER_SIZE];
	memset(bufferOutChar, 0, sizeof(bufferOutChar)); // zero out buffer
	for (i = 0; i < n; i++) {
		std::bitset<8> b(streamBufferOut.substr(i * 8, 8));
		bufferOutChar[i] = b.to_ulong();
	}
	streamBufferOut.erase(0, n * 8);
	streamOut.write(const_cast<const char*>(bufferOutChar), n);
	return true;
}

/*
 * Extractor Implementation
 */
void Extractor::set(shared_ptr<CharacterToBinaryTable> tablePtr) {
	characterToBinaryTablePtr = tablePtr;
}

void Extractor::set(shared_ptr<FileData> fileDataPtr_) {
	fileDataPtr = fileDataPtr_;
}

bool Extractor::stream(istream& streamIn, ostream& streamOut) {
	fileSizeCurrent = 0;
	fileSizeTarget = fileDataPtr->getFileSize();
	return scanStream(streamIn, &streamOut);
}
bool Extractor::bufferHandle(string& streamBuffer, bool final, va_list& args) {
	/* characters are placed into a hash table
	 * unordered_map[CharacterCode] = CharacterFrequency
	 */
	ostream& streamOut = va_arg(args, ostream);
	size_t i, j, nTotal, nBatches, nBatch, nLimit;
	nTotal = streamBuffer.size();
	nBatch = STREAM_SCANNER_BUFFER_SIZE / 8;
	nBatches = (nTotal / nBatch) + 1;
	for (i = 0; i < nBatches; i++) {
		nLimit = (i + 1) * nBatch;
		if (nLimit > nTotal) {
			nLimit = nTotal;
		}
		for (j = i * nBatch; j < nLimit; j++) {
			std::bitset<8> b(streamBuffer[j]);
			binaryString.append(b.to_string());
		}
		bufferBinaryHandle(false, streamOut);
	}
	streamBuffer.clear();
	if (final) {
		bufferBinaryHandle(true, streamOut);
		bufferOutHandle(true, streamOut);
		if (fileSizeCurrent != fileSizeTarget) {
			// original file size does not match the current
			throw 8;
		}
	}
	return true;
}

bool Extractor::bufferBinaryHandle(bool final, ostream& streamOut) {
	size_t i, n, last;
	unsigned int characterCode = 0;
	i = 1;
	last = 0;
	n = binaryString.size();
	while (i <= n && fileSizeCurrent < fileSizeTarget) {
		if (characterToBinaryTablePtr->binaryStringToCharacterCode(
				binaryString.substr(last, i), characterCode)) {
			char c[1];
			c[0] = static_cast<char>(characterCode);
			streamBufferOut.append(c, 1);
			last = last + i;
			n = n - i;
			i = 1;
			if (streamBufferOut.size() >= STREAM_SCANNER_BUFFER_SIZE) {
				bufferOutHandle(false, streamOut);
			}
			fileSizeCurrent++;
		} else {
			i++;
		}
	}
	if (last > 0) {
		binaryString.erase(0, last);
	}
	// we don't need to handle the leftover bits. they were only padding
	return true;
}

bool Extractor::bufferOutHandle(bool final, ostream& streamOut) {
	size_t nBuf = streamBufferOut.size();
	if (nBuf > STREAM_SCANNER_BUFFER_SIZE) {
		nBuf = STREAM_SCANNER_BUFFER_SIZE;
	}
	if (nBuf > 0) {
		streamOut.write(
				const_cast<const char*>(streamBufferOut.substr(0, nBuf).c_str()),
				nBuf);
		streamBufferOut.erase(0, nBuf);
	}
	// we don't need to handle the leftover bits. they were only padding
	return true;
}

/*
 * main & interface
 * Rules For Encoding:
 * - Read through file and increment character code frequency in a hash table based on contents
 * - Generate frequency table with priority queue
 * - Create binary tree from priority queue
 * - Encrypt the input file as an encrypted binary file
 *
 * Rules For Decoding
 * - Decrypt the encrypted binary file using the existing priority queue
 *
 */
int main() {
	FileHandler fh;
	CharacterPriorityQueue characterPriorityQueue;
	CharacterPriorityQueueTree characterPriorityQueueTree;
	shared_ptr<CharacterToBinaryTable> characterToBinaryTablePtr = make_shared<
			CharacterToBinaryTable>();
	shared_ptr<FileData> filedataPtr = make_shared<FileData>();
	Compressor compressor;
	Extractor extractor;
	string fileNameOriginal, fileNameEncrypt, fileNameDecrypt;
	ifstream fileStreamIn;
	ofstream fileStreamOut;
	/* input/output files are here */
	fileNameOriginal = "Speech.txt";
	fileNameEncrypt = "encrypt.data";
	fileNameDecrypt = "decrypt.txt";
	cout << "Opening the input file." << endl;
	try {
		fh.readStream(fileNameOriginal, fileStreamIn);
		fh.writeStream(fileNameEncrypt, fileStreamOut);
		cout << "Parsing input input stream as a character frequency table."
				<< endl;
		/* we pass a file stream instead of a reading the whole file
		 * into memory to reduce memory usage.
		 */
		characterPriorityQueue.stream(fileStreamIn);
		// get file data
		filedataPtr->stream(fileStreamIn);
		cout << "Building the priority queue." << endl;
		characterPriorityQueue.buildPriorityQueue();
		cout << "Building the priority queue tree." << endl;
		characterPriorityQueueTree.buildTree(
				characterPriorityQueue.getPriorityQueue());
		cout << "Building the binary string table." << endl;
		characterPriorityQueueTree.buildBinaryTable();
		cout << "Compressing the input file." << endl;
		characterToBinaryTablePtr->set(
				characterPriorityQueueTree.getCharacterToBinaryTable());
		compressor.set(characterToBinaryTablePtr);
		compressor.stream(fileStreamIn, fileStreamOut);
		/* encoding successful, now let's clean up */
		cout << "Closing input and output files." << endl;
		fh.close(fileStreamIn);
		fh.close(fileStreamOut);
		cout << "The file \"" << fileNameOriginal
				<< "\" was successfully compressed as \"" << fileNameEncrypt
				<< "\"" << endl;
		cout << "Opening the input file." << endl;
		fh.readStream(fileNameEncrypt, fileStreamIn);
		fh.writeStream(fileNameDecrypt, fileStreamOut);
		cout << "Extracting the input file." << endl;
		// only need for decoding
		characterToBinaryTablePtr->buildBinaryStringToCharacterCodeTable();
		extractor.set(characterToBinaryTablePtr);
		extractor.set(filedataPtr);
		extractor.stream(fileStreamIn, fileStreamOut);
		cout << "Closing input and output files." << endl;
		fh.close(fileStreamIn);
		fh.close(fileStreamOut);
		cout << "The file \"" << fileNameEncrypt
				<< "\" was successfully extracted as \"" << fileNameDecrypt
				<< "\"" << endl;
	} catch (int& exceptionCode) {
		switch (exceptionCode) {
		case 1:
			cout << "[Error] Could not open the input file \""
					<< fileNameOriginal << "\"" << endl;
			break;
		case 2:
			cout << "[Error] Could not open the output file \""
					<< fileNameEncrypt << "\"" << endl;
			break;
		case 3:
			cout
					<< "[Error] Could not parse the input stream as a character frequency table."
					<< endl;
			break;
		case 4:
			cout << "[Error] Could not build the priority queue." << endl;
			break;
		case 5:
			cout << "[Error] Could not build the priority queue tree." << endl;
			break;
		case 6:
			cout << "[Error] Could not build the binary string table." << endl;
			break;
		case 7:
			cout << "[Error] Could not compress the file." << endl;
			break;
		case 8:
			cout << "[Error] Could not extract the file." << endl;
			break;
		case 9:
			cout << "[Error] Could not close the input file \""
					<< fileNameOriginal << "\"" << endl;
			break;
		case 10:
			cout << "[Error] Could not close the output file \""
					<< fileNameEncrypt << "\"" << endl;
			break;
		}
	}

	cout << "Enter any key to exit..." << endl;
	string temp;
	getline(cin, temp);
	return 0;
}
