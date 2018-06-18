//============================================================================
// Name        : Lab5
// Author      : Branden Lee
// Date        : 5/30/2018
// Description : Encryption and Compression
//============================================================================

/**
 * Project Assessment:
 * In a preliminary overview of the project and the input file, I determined
 * that the amalgamated HTML document "PageRank.html" is not valid HTML and is missing many closing tags.
 * Despite this, I have attempted to use my general XML parser from lab 3
 * to parse the document and find the outbound link nodes
 */

#include <string>
#include <fstream>
#include <iostream>			// std::cout
#include <sstream>
#include <iomanip>
#include <vector>			// std::vector
#include <stack>
#include <queue>			// std::priority_queue
#include <memory>			// std::unique_ptr
#include <unordered_map>	// std::unordered_map
#include <regex>			// std::regex_match
#include <functional>		// std::function
#include <string_view>		// std::string_view
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
	void readStream(string fileName, ifstream& fileStream) throw (unsigned int);
	void writeStream(string fileName, ofstream& fileStream) throw (unsigned int);
	void writeString(string fileName, string stringValue) throw (unsigned int);
	void close(ifstream& fileStream) throw (unsigned int);
	void close(ofstream& fileStream) throw (unsigned int);
};

/**
 * @class StreamScanner
 * A reusable class to scan and parse a stream \n
 */
class StreamScanner {
public:
	static bool scanStream(istream& streamIn,
			std::function<void(string&, bool)> bufferHandleCallback);
};

/**
 @class XMLNode
 XML document node \n
 */
class XMLNode: public std::enable_shared_from_this<XMLNode> {
private:
	string name_; // tag name inside the angled brackets <name>
	string value_; // non-child-node inside node <>value</>
	vector<shared_ptr<XMLNode>> childNodes;
	/* 2018-06-04 Revision 2
	 * XMLNode.parentNode was removed because it could lead to circular references
	 * */
	regex tagOpenRegex, tagEndRegex;
public:
	XMLNode() :
			name_(""), value_(""), tagOpenRegex("\\<(.*?)\\>"), tagEndRegex(
					"\\<\\/(.*?)\\>") {
	}
	XMLNode(string name) :
			name_(name), value_(""), tagOpenRegex("\\<(.*?)\\>"), tagEndRegex(
					"\\<\\/(.*?)\\>") {
	}
	~XMLNode() {

	}
	bool parseStream(ifstream& streamIn);
	bool parseStream(istream& streamIn);
	bool streamBufferHandle(string& streamBuffer, bool final,
			stack<shared_ptr<XMLNode>>& documentStack, unsigned int& mode);
	bool nodePop(string& tagString, stack<shared_ptr<XMLNode>>& documentStack);
	bool nodePush(string& tagString, stack<shared_ptr<XMLNode>>& documentStack);
	void valueAppend(string str);
	/* not a comprehensive list of definitions for all getters/setters
	 * it is not vital to the program to have all setters
	 */
	string getName();
	string getValue();
	/* 2018-06-04 Revision 2
	 * XMLNode.getParent() was removed because it could lead to circular references
	 * */
	shared_ptr<XMLNode> addChild(string str);
	shared_ptr<XMLNode> getChild(unsigned int index);
	bool findChild(string name, shared_ptr<XMLNode>& returnNode,
			unsigned int index);
	unsigned int childrenSize();
	unsigned int findChildSize(string name);
};

/**
 * @class HTMLDocument
 */
class HTMLDocument {
private:
	string title_;
	unsigned int linkOutNum_, linkInNum_;
	vector<shared_ptr<string>> linkOutList;
public:
	void setTitle(string title);
	void setLinkOutNum(unsigned int linkOutNum);
	void setLinkInNum(unsigned int linkInNum);
	string getTitle();
	unsigned int getLinkOutNum();
	unsigned int getLinkInNum();
	void addLinkOut(string title);
	vector<shared_ptr<string>> getLinkOutList();
};

/**
 * @class DocumentDatabase
 */
class DocumentDatabase {
private:
	vector<shared_ptr<vector<bool>>> adjacencyMatrix;
	vector<shared_ptr<HTMLDocument>> documentList;
public:
	void addDocument(shared_ptr<HTMLDocument> document);
	void createGraph();
	void calculatePageRank();
	string getAllPageRank();
};

/**
 * @class DocumentExtractor
 * Assists in extracting relevant information
 */
class DocumentExtractor {
public:
	void extract(XMLNode& XML_Document, DocumentDatabase& documentDatabase);
};

/*
 * FileHandler Implementation
 */
void FileHandler::readStream(string fileName, ifstream& fileStream)
		throw (unsigned int) {
	fileStream.open(fileName, ios::binary);
	if (!fileStream.is_open()) {
		throw 1;
	}
}

void FileHandler::writeStream(string fileName, ofstream& fileStream)
		throw (unsigned int) {
	fileStream.open(fileName, ios::binary);
	if (!fileStream.is_open()) {
		throw 2;
	}
}

void FileHandler::writeString(string fileName, string stringValue)
		throw (unsigned int) {
	ofstream fileStream;
	fileStream.open(fileName);
	if (fileStream.is_open()) {
		fileStream << stringValue;
		fileStream.close();
	} else {
		throw 1;
	}
}

void FileHandler::close(ifstream& fileStream) throw (unsigned int) {
	try {
		fileStream.close();
	} catch (...) {
		throw 7;
	}
}

void FileHandler::close(ofstream& fileStream) throw (unsigned int) {
	try {
		fileStream.close();
	} catch (...) {
		throw 8;
	}
}

/*
 * StreamScanner Implementation
 */
bool StreamScanner::scanStream(istream& streamIn,
		std::function<void(string&, bool)> bufferHandleCallback) {
	unsigned int fileSize, filePos, bufferSize, mode;
	string streamBuffer;
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
		bufferHandleCallback(streamBuffer, false);
		// advance buffer
		filePos += bufferSize;
	}
	// handle the remaining buffer
	bufferHandleCallback(streamBuffer, true);
	return true;
}

/*
 * XMLNode Implementation
 */
bool XMLNode::parseStream(ifstream& streamIn) {
	return parseStream(static_cast<istream&>(streamIn));
}
bool XMLNode::parseStream(istream& streamIn) {
	unsigned int mode = 0;
	stack<shared_ptr<XMLNode>> documentStack;
	/* 2018-06-04 Revision 2
	 * bottom of the stack is the document node.
	 * */
	documentStack.push(shared_from_this());
	return StreamScanner::scanStream(streamIn,
			[this, &mode, &documentStack](string& streamBuffer, bool final) {
				streamBufferHandle(streamBuffer, final, documentStack, mode);
			});
}
bool XMLNode::streamBufferHandle(string& streamBuffer, bool final,
		stack<shared_ptr<XMLNode>>& documentStack, unsigned int& mode) {
	size_t tagOpenPos, tagEndPos;
	string tagPop, matchGroupString, temp;
	while (true) {
		if (mode == 0) {
			// Expecting opening angle bracket for a tag
			tagOpenPos = (unsigned int) streamBuffer.find("<");
			if (tagOpenPos != string::npos) {
				/* opening angle bracket for a tag
				 * we assume that text before this is the value of current xml node
				 */
				mode = 1;
				documentStack.top()->valueAppend(
						streamBuffer.substr(0, tagOpenPos));
				streamBuffer.erase(0, tagOpenPos);
				tagOpenPos = 0;
			} else {
				break;
			}
		} else if (mode == 1) {
			// expecting ending angle bracket for a tag
			tagEndPos = (unsigned int) streamBuffer.find(">");
			temp = streamBuffer.substr(0, tagEndPos + 1);
			std::smatch m;
			if (tagEndPos != string::npos) {
				// let's use regex to grab the tag name between the angled brackets
				// let's first check if we just ended an ending tag </>
				//std::smatch m;
				regex_match(temp, m, tagEndRegex);
				if (!m.empty()) {
					/* extract matched group
					 * a .trim() method would be great...
					 */
					try {
						matchGroupString = m[1].str(); // match group
					} catch (...) {
						matchGroupString = "";
					}
					/*string s;
					 s.append("</").append(matchGroupString).append(">\n");
					 cout << s;*/
					documentStack.top()->nodePop(matchGroupString,
							documentStack);
				} else {
					// now check if we just ended an opening tag <>
					//std::smatch m;
					regex_match(temp, m, tagOpenRegex);
					if (!m.empty()) {
						try {
							matchGroupString = m[1].str(); // match group
						} catch (...) {
							matchGroupString = "";
						}
						/*string s;
						 s.append("<").append(matchGroupString).append(">\n");
						 cout << s;*/
						documentStack.top()->nodePush(matchGroupString,
								documentStack);
					}
				}
				// erase to the end of the ending angle bracket ">"
				streamBuffer.erase(0, tagEndPos + 1);
				mode = 0;
			} else {
				break;
			}
		}
	}
	if (final) {
		documentStack.top()->valueAppend(streamBuffer);
	}
	return true;
}

bool XMLNode::nodePop(string& tagString,
		stack<shared_ptr<XMLNode>>& documentStack) {
	/* pop nodes off stack until end tag is found
	 * can't go lower than the document root
	 */
	string tagPop = "";
	if (tagString.length() > 0) {
		/* 2018-06-04 Revision 2
		 * bottom of the stack is the document node. These reduces number of arguments passed.
		 * */
		while (documentStack.size() > 1 && tagPop != tagString) {
			tagPop = documentStack.top()->getName();
			documentStack.pop();
			/* 2018-06-04 Revision 2
			 * XMLNode.getParent() was removed because it could lead to circular references
			 * */
		}
	}
	return true;
}

bool XMLNode::nodePush(string& tagString,
		stack<shared_ptr<XMLNode>>& documentStack) {
	if (tagString.length() > 0) {
		documentStack.push(documentStack.top()->addChild(tagString));
	}
	return true;
}
void XMLNode::valueAppend(string str) {
	value_.append(str);
}
string XMLNode::getName() {
	return name_;
}
string XMLNode::getValue() {
	return value_;
}
shared_ptr<XMLNode> XMLNode::addChild(string str) {
	shared_ptr<XMLNode> childNode = make_shared<XMLNode>(str);
	//cout << "child " << str << " name: "<<name << endl;
	childNodes.push_back(childNode);
	return childNode;
}
shared_ptr<XMLNode> XMLNode::getChild(unsigned int index) {
	shared_ptr<XMLNode> nodeReturn;
	try {
		nodeReturn = childNodes.at(index);
	} catch (...) {
		// nothing
	}
	return nodeReturn;
}
bool XMLNode::findChild(string name, shared_ptr<XMLNode>& returnNode,
		unsigned int index) {
	unsigned int findCount, i, n;
	bool returnValue = false;
	findCount = 0;
	n = static_cast<unsigned int>(childNodes.size());
	for (i = 0; i < n; i++) {
		if (childNodes[i]->name_ == name) {
			if (findCount == index) {
				returnNode = childNodes[i];
				returnValue = true;
				break;
			}
			findCount++;
		}
	}
	return returnValue;
}
unsigned int XMLNode::childrenSize() {
	return static_cast<unsigned int>(childNodes.size());
}
unsigned int XMLNode::findChildSize(string name) {
	unsigned int findCount, i, n;
	findCount = 0;
	n = static_cast<unsigned int>(childNodes.size());
	for (i = 0; i < n; i++) {
		if (childNodes[i]->name_ == name) {
			findCount++;
		}
	}
	return findCount;
}

/*
 * HTMLDocument Implementation
 */

/*
 * DocumentDatabase Implementation
 */
void DocumentDatabase::createGraph() {

}

void DocumentDatabase::calculatePageRank() {

}

string DocumentDatabase::getAllPageRank() {
	return "";
}

/*
 * DocumentExtractor Implementation
 */
void DocumentExtractor::extract(XMLNode& XML_Document,
		DocumentDatabase& documentDatabase) {

}

/*
 * main & interface
 * Rules for calculating page rank:
 * - parse the poorly formed "HTML" document
 * - Generate frequency table with priority queue
 * - Create binary tree from priority queue
 * - Encrypt the input file as an encrypted binary file
 */
int main() {
	FileHandler fh;
	XMLNode document;
	DocumentExtractor documentExtractor;
	DocumentDatabase documentDatabase;
	string fileNameHTML;
	ifstream fileStreamIn;
	/* input/output files are here */
	fileNameHTML = "PageRank.html";
	cout << "Opening the input file.\n";
	try {
		fh.readStream(fileNameHTML, fileStreamIn);
		cout << "Parsing the HTML file. \"" << fileNameHTML << "\"\n";
		/* we pass a file stream instead of a reading the whole file
		 * into memory to reduce memory usage.
		 */
		document.parseStream(fileStreamIn);
		cout << "Closing input file.\n";
		fh.close(fileStreamIn);
		cout << "Extracting relevant XML data to the document database.\n";
		documentExtractor.extract(document, documentDatabase);
		/* The Document Extractor takes an XML document and extracts
		 * the lab specific information into a document database
		 */
		cout << "Creating graph from documents.\n";
		documentDatabase.createGraph();
		cout << "Calculating Page Ranks.\n";
		documentDatabase.calculatePageRank();
		cout << "Page rank:\n";
		cout << documentDatabase.getAllPageRank() << "\n";
	} catch (int& exceptionCode) {
		switch (exceptionCode) {
		case 1:
			cout << "[Error] Could not open the input file \"" << fileStreamIn
					<< "\"\n";
			break;
		case 2:
			cout << "[Error] Could not open the output file\n";
			break;
		case 3:
			cout
					<< "[Error] Could not parse the input stream as a character frequency table.\m";
			break;
		case 4:
			cout << "[Error] Could not build the priority queue.\n";
			break;
		case 5:
			cout << "[Error] Could not build the priority queue tree.\n";
			break;
		case 6:
			cout << "[Error] Could not build the binary string table.\n";
			break;
		case 7:
			cout << "[Error] Could not compress the file.\n";
			break;
		case 8:
			cout << "[Error] Could not extract the file.\n";
			break;
		case 9:
			cout << "[Error] Could not close the input file \"" << fileStreamIn
					<< "\"\n";
			break;
		}
	}

	cout << "Enter any key to exit...\n";
	string temp;
	getline(cin, temp);
	return 0;
}
