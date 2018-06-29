//============================================================================
// Name        : Lab5
// Author      : Branden Lee
// Date        : 6/29/2018
// Description : Graphs
//============================================================================

/**
 * IMPORTANT NOTE: THIS PROJECT USES C++17
 * In Microsoft Visual Studio 2017 set C++17 by navigating to:
 * project -> properties -> C++ -> language
 *
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
#include <algorithm>    	// std::find
#include <locale>			// std::locale, std::tolower
#include <sstream>			// std::stringstream
#include <cmath>			// std::abs
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
	 @throw unsigned int 1 error code
	 */
	void readStream(string fileName, ifstream& fileStream);
	void writeStream(string fileName, ofstream& fileStream);
	void writeString(string fileName, string stringValue);
	void close(ifstream& fileStream);
	void close(ofstream& fileStream);
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
 * @class HTMLNode
 * HTML document node \n
 * HTML is not easy to parse. There are many if/else statements to catch different cases
 * for html nodes. some html nodes don't have ending tags, so they must watched for. \n
 * This node class will also be used for the document node and DOM search results.
 */
class HTMLNode {
public:
	using item_t = shared_ptr<HTMLNode>;
	using container_t = vector<item_t>;
private:
	string name_; // tag name inside the angled brackets <name>
	string attributes_; // <name attribute="attributeValue">
	string value_; // non-child-node inside node <>value</>
	container_t childNodes;
	/* 2018-06-04 Revision 2
	 * HTMLNode.parentNode was removed because it could lead to circular references
	 * */
	regex tagOpenRegex, tagEndRegex;
public:
	HTMLNode() :
			name_(""), value_(""), tagOpenRegex("\\<(.*?)\\>"), tagEndRegex(
					"\\<\\/(.*?)\\>") {
	}
	HTMLNode(string name) :
			name_(name), value_(""), tagOpenRegex("\\<(.*?)\\>"), tagEndRegex(
					"\\<\\/(.*?)\\>") {
	}
	~HTMLNode() {

	}
	bool parseStream(ifstream& streamIn);
	bool parseStream(istream& streamIn);
	bool streamBufferHandle(string& streamBuffer, bool final,
			stack<item_t>& documentStack, unsigned int& mode);
	bool nodePop(string tagString, stack<item_t>& documentStack);
	bool nodePush(string tagString, stack<item_t>& documentStack);
	void setAttributes(string str);
	void valueAppend(string str);
	/* not a comprehensive list of definitions for all getters/setters
	 * it is not vital to the program to have all setters
	 */
	string getName();
	string getAttributes();
	string getValue();
	/* 2018-06-04 Revision 2
	 * HTMLNode.getParent() was removed because it could lead to circular references
	 * */
	item_t addChild(string str);

	item_t& at(unsigned long long int index) {
		return childNodes.at(index);
	}
	void push_back(item_t item) {
		childNodes.push_back(item);
	}
	container_t::iterator find(string& name) {
		return find_if(childNodes.begin(), childNodes.end(),
				[name](const shared_ptr<HTMLNode>& arg)->bool {
					return arg->getName() == name;
				});
	}
	void findName(HTMLNode& matches, string str);
	void findAttribute(HTMLNode& matches, string str);
	void findValue(HTMLNode& matches, string str);
	string toString(string prefix = "");
	size_t size() {
		return childNodes.size();
	}
	container_t::iterator begin() {
		return childNodes.begin();
	}
	container_t::iterator end() {
		return childNodes.end();
	}
	bool operator==(const item_t& rhs) {
		return getName() == rhs->getName();
	}
	bool operator==(const string& rhs) {
		return getName() == rhs;
	}
};

/**
 * @class HTMLDocument
 * The HTML document used in page rank calculations
 */
class HTMLDocument {
public:
	using container_t = vector<shared_ptr<HTMLDocument>>;
private:
	string title_, link_;
	container_t documentOutList;
public:
	unsigned int linkOutNum, linkInNum;
	double pageRank, pageRankLast, pageRankDifference;
	HTMLDocument(string title) :
			title_(title), linkOutNum(0), linkInNum(0), pageRank(1.0), pageRankLast(
					1.0), pageRankDifference(0.0) {

	}
	void setTitle(string title);
	void setLink(string str);
	string getTitle();
	string getLink();
	void addDocumentOut(shared_ptr<HTMLDocument> docPtr) {
		documentOutList.push_back(docPtr);
	}
	size_t pageOutNum() {
		return documentOutList.size();
	}
	container_t::iterator begin() {
		return documentOutList.begin();
	}
	container_t::iterator end() {
		return documentOutList.end();
	}
	string toString(string prefix = "", unsigned int depth = 1);
	string calculationToString();
};

/**
 * @class DocumentDatabase
 * This class contains the database of webpages we will use for page rank calculations. \n
 * The table key is case insensitive.
 */
class DocumentDatabase {
public:
	using item_t = shared_ptr<HTMLDocument>;
	using container_t = unordered_map<string, item_t>;
private:
	vector<shared_ptr<vector<bool>>> adjacencyMatrix;
	container_t documentTable;
	std::locale loc;
	unsigned int pageRankIterations;
public:
	double pageRankDifferenceTotal, dampingFactor;
	DocumentDatabase() :
			pageRankIterations(0), pageRankDifferenceTotal(0.0), dampingFactor(
					0.85) {
	}
	void insert(string key, shared_ptr<HTMLDocument> item) {
		std::transform(key.begin(), key.end(), key.begin(), ::tolower);
		documentTable.insert( { key, item });
	}
	void createGraph();
	void calculatePageRank();
	void calculatePageRankIteration(double d);
	string getAllPageRank();
	container_t::iterator find(string key) {
		std::transform(key.begin(), key.end(), key.begin(), ::tolower);
		return documentTable.find(key);
	}
	container_t::iterator begin() {
		return documentTable.begin();
	}
	container_t::iterator end() {
		return documentTable.end();
	}
	string toString(string prefix = "", unsigned int depth = 1);
	string calculationToString();
	string graphToString();
};

/**
 * @class DocumentExtractor
 * Assists in extracting relevant information from the HTML DOM to the
 * document database
 */
class DocumentExtractor {
public:
	void extract(HTMLNode& XML_Document, DocumentDatabase& documentDatabase);
};

/*
 * FileHandler Implementation
 */
void FileHandler::readStream(string fileName, ifstream& fileStream) {
	fileStream.open(fileName, ios::binary);
	if (!fileStream.is_open()) {
		throw 1;
	}
}

void FileHandler::writeStream(string fileName, ofstream& fileStream) {
	fileStream.open(fileName, ios::binary);
	if (!fileStream.is_open()) {
		throw 2;
	}
}

void FileHandler::writeString(string fileName, string stringValue) {
	ofstream fileStream;
	fileStream.open(fileName);
	if (fileStream.is_open()) {
		fileStream << stringValue;
		fileStream.close();
	} else {
		throw 1;
	}
}

void FileHandler::close(ifstream& fileStream) {
	try {
		fileStream.close();
	} catch (...) {
		throw 3;
	}
}

void FileHandler::close(ofstream& fileStream) {
	try {
		fileStream.close();
	} catch (...) {
		throw 4;
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
 * HTMLNode Implementation
 */
bool HTMLNode::parseStream(ifstream& streamIn) {
	return parseStream(static_cast<istream&>(streamIn));
}
bool HTMLNode::parseStream(istream& streamIn) {
	unsigned int mode = 0;
	stack<shared_ptr<HTMLNode>> documentStack;
	/* 2018-06-04 Revision 2
	 * bottom of the stack is the document node.
	 * */
	return StreamScanner::scanStream(streamIn,
			[this, &mode, &documentStack](string& streamBuffer, bool final) {
				streamBufferHandle(streamBuffer, final, documentStack, mode);
			});
}
bool HTMLNode::streamBufferHandle(string& streamBuffer, bool final,
		stack<shared_ptr<HTMLNode>>& documentStack, unsigned int& mode) {
	size_t tagOpenPos, tagEndPos;
	string tagPop, temp;
	while (true) {
		if (mode == 0) {
			// Expecting opening angle bracket for a tag
			tagOpenPos = streamBuffer.find("<");
			if (tagOpenPos != string::npos) {
				/* opening angle bracket for a tag
				 * we assume that text before this is the value of current xml node
				 */
				mode = 1;
				if (documentStack.empty()) {
					valueAppend(streamBuffer.substr(0, tagOpenPos));
				} else {
					documentStack.top()->valueAppend(
							streamBuffer.substr(0, tagOpenPos));
				}
				streamBuffer.erase(0, tagOpenPos);
				tagOpenPos = 0;
			} else {
				break;
			}
		} else if (mode == 1) {
			if (documentStack.empty()) {
				//cout << "last tag: none\n";
			} else {
				//cout << "last tag: " << documentStack.top()->getName() << "\n";
				//cout << "BUFFER: " << streamBuffer << "\n";
			}
			// expecting ending angle bracket for a tag
			tagEndPos = streamBuffer.find(">");
			//std::smatch m;
			if (tagEndPos != string::npos) {
				// let's use regex to grab the tag name between the angled brackets
				// let's first check if we just ended an ending tag </>
				//std::smatch m;
				//regex_match(temp, m, tagEndRegex);
				temp = streamBuffer.substr(1, tagEndPos - 1);
				if (temp.length() > 0 && temp.substr(0, 1) == "/") {
					/* extract matched group
					 * a .trim() method would be great...
					 */
					// tag name is the first word
					string tagName = temp.substr(1, temp.find(' '));
					//cout << "</tag>: " << tagName << "\n";
					/*string s;
					 s.append("</").append(matchGroupString).append(">\n");
					 cout << s;*/
					nodePop(tagName, documentStack);
				} else if (temp.length() > 0) {
					/* now check if we just ended an opening tag <>
					 * if the last tagName was "script"
					 * we check if the new tag is recognizable, and if not
					 * then just add the buffer up to ">" for the current node
					 * 2018-06-29 regex keeps crashing at this line, so we removed it
					 */
					//regex_match(temp, m, tagOpenRegex);
					// tag name is the first word
					size_t spacePosFirst = temp.find(' ');
					string tagName = temp.substr(0, spacePosFirst);
					if (!documentStack.empty()
							&& documentStack.top()->getName() == "script"
							&& tagName != "div") {
						// add to script node value
						documentStack.top()->valueAppend(
								streamBuffer.substr(0, tagEndPos + 1));
					} else {
						if (!documentStack.empty()
								&& documentStack.top()->getName() == "script") {
							// get out of script node
							nodePop(documentStack.top()->getName(),
									documentStack);
						}
						// add the new node
						string attributes;
						if (spacePosFirst != string::npos) {
							attributes = temp.substr(spacePosFirst + 1);
						}
						// pop html node before inserting html node. no nested html
						if (tagName == "html") {
							nodePop(tagName, documentStack);
						}
						if (documentStack.empty()) {
							nodePush(tagName, documentStack);
						} else {
							documentStack.top()->nodePush(tagName,
									documentStack);
						}
						documentStack.top()->setAttributes(attributes);
						// End nodes that do not require end tag
						if (temp.substr(temp.length() - 1) == "/"
								|| temp.substr(0, 1) == "!" || tagName == "meta"
								|| tagName == "link" || tagName == "br"
								|| tagName == "input" || tagName == "img") {
							nodePop(tagName, documentStack);
						}
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
		if (documentStack.empty()) {
			valueAppend(streamBuffer);
		} else {
			documentStack.top()->valueAppend(streamBuffer);
		}
	}
	return true;
}

bool HTMLNode::nodePop(string tagString,
		stack<shared_ptr<HTMLNode>>& documentStack) {
	/* pop nodes off stack until end tag is found
	 * can't go lower than the document root
	 */
	string tagPop = "";
	if (!documentStack.empty()) {
		tagPop = documentStack.top()->getName();
	}
	if (tagString.length() > 0) {
		/* 2018-06-04 Revision 2
		 * bottom of the stack is the document node. These reduces number of arguments passed.
		 * */
		while (!documentStack.empty() && tagPop != tagString && tagPop != "html") {
			documentStack.pop();
			if (!documentStack.empty()) {
				tagPop = documentStack.top()->getName();
			}
			/* 2018-06-04 Revision 2
			 * HTMLNode.getParent() was removed because it could lead to circular references
			 * */
		}
		if (!documentStack.empty() && tagPop == tagString) {
			documentStack.pop();
		}
	}
	return true;
}

bool HTMLNode::nodePush(string tagString,
		stack<shared_ptr<HTMLNode>>& documentStack) {
	if (tagString.length() > 0) {
		if (documentStack.empty()) {
			documentStack.push(addChild(tagString));
		} else {
			documentStack.push(documentStack.top()->addChild(tagString));
		}
	}
	return true;
}
void HTMLNode::setAttributes(string str) {
	attributes_ = str;
}
void HTMLNode::valueAppend(string str) {
	value_.append(str);
}
string HTMLNode::getName() {
	return name_;
}
string HTMLNode::getAttributes() {
	return attributes_;
}
string HTMLNode::getValue() {
	return value_;
}
shared_ptr<HTMLNode> HTMLNode::addChild(string str) {
	shared_ptr<HTMLNode> childNode = make_shared<HTMLNode>(str);
	//cout << "child " << str << " name: "<<name << endl;
	childNodes.push_back(childNode);
	return childNode;
}
void HTMLNode::findName(HTMLNode& matches, string str) {
	for (auto i : childNodes) {
		if (i->getName().find(str) != string::npos) {
			matches.push_back(i);
		}
		i->findName(matches, str);
	}
}
void HTMLNode::findAttribute(HTMLNode& matches, string str) {
	for (auto i : childNodes) {
		if (i->getAttributes().find(str) != string::npos) {
			matches.push_back(i);
		}
		i->findAttribute(matches, str);
	}
}
void HTMLNode::findValue(HTMLNode& matches, string str) {
	for (auto i : childNodes) {
		if (i->getValue().find(str) != string::npos) {
			matches.push_back(i);
		}
		i->findValue(matches, str);
	}
}
string HTMLNode::toString(string prefix) {
	string output;
	for (auto i : childNodes) {
		output.append(prefix).append(i->getName()).append(" ").append(
				i->getAttributes().substr(0, 30));
		output.append(" ").append(i->getValue().substr(0, 30)).append("\n");
		output.append(i->toString(prefix + "\t"));
	}
	return output;
}

/*
 * HTMLDocument Implementation
 */
void HTMLDocument::setTitle(string str) {
	title_ = str;
}
void HTMLDocument::setLink(string str) {
	link_ = str;
}
string HTMLDocument::getTitle() {
	return title_;
}
string HTMLDocument::getLink() {
	return link_;
}
string HTMLDocument::toString(string prefix, unsigned int depth) {
	stringstream output;
	output << prefix << left << setw(20) << setfill(' ')
			<< getTitle().substr(0, 19) << setw(10) << fixed << setprecision(6)
			<< pageRank << getLink();
	if (depth > 0) {
		for (auto docOutIt : documentOutList) {
			output << "\n";
			output << docOutIt->toString(prefix + "\t", depth - 1);
		}
	}
	return output.str();
}
string HTMLDocument::calculationToString() {
	stringstream output;
	output << left << setw(20) << setfill(' ') << getTitle().substr(0, 19)
			<< setw(10) << fixed << setprecision(6) << pageRank << setw(10)
			<< pageRankLast << setw(10) << pageRankDifference;
	return output.str();
}

/*
 * DocumentDatabase Implementation
 */
void DocumentDatabase::createGraph() {
	for (auto docItX : documentTable) {
		size_t n = adjacencyMatrix.size();
		adjacencyMatrix.push_back(make_shared<vector<bool>>());
		for (auto docItY : documentTable) {
			string titleX = docItX.second->getTitle();
			string titleY = docItY.second->getTitle();
			auto foundIt = find_if(docItX.second->begin(), docItX.second->end(),
					[titleY](const shared_ptr<HTMLDocument> arg)->bool {
						return arg->getTitle() == titleY;
					});
			if (foundIt == docItX.second->end()) {
				//cout << titleX << " : " << titleY << " not found\n";
				adjacencyMatrix[n]->push_back(false);
			} else {
				//cout << titleX << " : " << titleY << " found\n";
				adjacencyMatrix[n]->push_back(true);
			}
		}
	}
}

void DocumentDatabase::calculatePageRank() {
	// precision of 3 decimal places
	while (pageRankDifferenceTotal > 0.001 || pageRankIterations < 4) {
		// One calculation
		calculatePageRankIteration(dampingFactor);
		cout << calculationToString() << "\n\n";
	}
}

void DocumentDatabase::calculatePageRankIteration(double d) {
	// One calculation
	size_t x = 0;
	pageRankDifferenceTotal = 0;
	pageRankIterations++;
	for (auto graphXIt : documentTable) {
		size_t y = 0;
		double pageRank = 0;
		// += inbound document page rank / inbound document oubound links #
		for (auto graphYIt : documentTable) {
			if (adjacencyMatrix[x]->at(y)) {
				if (graphYIt.second->pageOutNum() != 0) {
					pageRank += graphYIt.second->pageRank
							/ graphYIt.second->pageOutNum();
				}
			}
			y++;
		}
		// *d + (1-d)
		x++;
		pageRank *= d;
		pageRank += 1 - d;
		graphXIt.second->pageRankLast = graphXIt.second->pageRank;
		graphXIt.second->pageRank = pageRank;
		graphXIt.second->pageRankDifference = abs(
				graphXIt.second->pageRankLast - graphXIt.second->pageRank);
		pageRankDifferenceTotal += graphXIt.second->pageRankDifference;
	}
}

string DocumentDatabase::getAllPageRank() {
	return "";
}

string DocumentDatabase::toString(string prefix, unsigned int depth) {
	stringstream output;
	output << left << setw(20) << setfill(' ') << "Title" << setw(10)
			<< "Page Rank" << "URL\n";
	for (auto docIt : documentTable) {
		output << docIt.second->toString("", depth) << "\n";
	}
	return output.str();
}

string DocumentDatabase::calculationToString() {
	stringstream output;
	output << "Iteration: #" << pageRankIterations << "\n";
	output << left << setw(20) << setfill(' ') << "Title" << setw(10)
			<< "Page Rank" << setw(10) << "Last" << "Difference\n";
	for (auto docIt : documentTable) {
		output << docIt.second->calculationToString() << "\n";
	}
	output << setw(50) << setfill('-') << "" << "\n";
	output << left << fixed << setprecision(6) << setw(20) << setfill(' ')
			<< "Total" << setw(20) << "" << pageRankDifferenceTotal;
	return output.str();
}

string DocumentDatabase::graphToString() {
	stringstream output;
// header
	output << setw(4) << "";
	for (auto graphXIt : documentTable) {
		output << setw(4) << graphXIt.second->getTitle().substr(0, 3);
	}
// content
	size_t n = 0;
	for (auto graphYIt : documentTable) {
		output << "\n" << setw(4) << graphYIt.second->getTitle().substr(0, 3);
		for (auto matrixIt : *adjacencyMatrix[n]) {
			output << setw(4);
			if (matrixIt) {
				output << "X";
			} else {
				output << "O";
			}
		}
		n++;
	}
	return output.str();
}

/*
 * DocumentExtractor Implementation
 */
void DocumentExtractor::extract(HTMLNode& XML_Document,
		DocumentDatabase& documentDatabase) {
	regex hrefRegex("href=\"(.*?)\"");
	std::smatch m;
	HTMLNode htmlMatches;
	XML_Document.findName(htmlMatches, "html");
	for (auto htmlDocIt : htmlMatches) {
		// find title
		string title;
		HTMLNode titleMatches;
		htmlDocIt->findName(titleMatches, "title");
		// Intentional overwrite
		for (auto htmlDocTitleIt : titleMatches) {
			title = htmlDocTitleIt->getValue();
		}
		// add document
		shared_ptr<HTMLDocument> HTML_document;
		auto htmlDocFoundIt = documentDatabase.find(title);
		if (htmlDocFoundIt == documentDatabase.end()) {
			// new document
			HTML_document = make_shared<HTMLDocument>(title);
			documentDatabase.insert(title, HTML_document);
		} else {
			// existing document
			HTML_document = htmlDocFoundIt->second;
		}
		// find outbound links
		HTMLNode divMatches;
		htmlDocIt->findAttribute(divMatches, "class=\"other-links\"");
		HTMLNode aMatches;
		divMatches.findName(aMatches, "a");
		//cout << aMatches.toString() << "\n";
		for (auto aIt : aMatches) {
			string titleOut = aIt->getValue();
			if (titleOut.length() > 0) {
				shared_ptr<HTMLDocument> HTML_documentOut;
				auto HTML_documentIterator = documentDatabase.find(titleOut);
				if (HTML_documentIterator == documentDatabase.end()) {
					// add outbound document
					HTML_documentOut = make_shared<HTMLDocument>(titleOut);
					documentDatabase.insert(titleOut, HTML_documentOut);
				} else {
					// existing document
					HTML_documentOut = HTML_documentIterator->second;
				}
				// set the document link
				string attributeStr = aIt->getAttributes();
				regex_match(attributeStr, m, hrefRegex);
				if (m.length() > 1) {
					HTML_documentOut->setLink(m[1]);
				}
				HTML_document->addDocumentOut(HTML_documentOut);
			}
		}
	}
}

/*
 * main & interface
 * Rules for calculating page rank:
 * - parse the poorly formed "HTML" document
 * - Extract relevant document data
 * - Generate adjacency matrix
 * - Calculate page rank
 * - Display results on console
 */
int main() {
	FileHandler fh;
	HTMLNode document;
	DocumentExtractor documentExtractor;
	DocumentDatabase documentDatabase;
	string fileNameHTML;
	ifstream fileStreamIn;
	/* input/output files are here */
	fileNameHTML = "PageRank.html";
	cout << "Opening the input file.\n";
	try {
		fh.readStream(fileNameHTML, fileStreamIn);
		cout << "Parsing the HTML file \"" << fileNameHTML << "\".\n";
		/* we pass a file stream instead of a reading the whole file
		 * into memory to reduce memory usage.
		 */
		document.parseStream(fileStreamIn);
		cout << "Closing input file.\n";
		fh.close(fileStreamIn);
		cout << "Extracting relevant XML data to the document database.\n";
		/* The Document Extractor takes an XML document and extracts
		 * the lab specific information into a document database
		 */
		documentExtractor.extract(document, documentDatabase);
		//cout << documentDatabase.toString() << "\n";
		cout << "Creating graph from documents.\n";
		documentDatabase.createGraph();
		cout << documentDatabase.graphToString() << "\n\n";
		cout << "Calculating Page Ranks.\n";
		documentDatabase.calculatePageRank();
		cout << "Page rank:\n";
		cout << documentDatabase.toString("", 0) << "\n";
	} catch (int& exceptionCode) {
		switch (exceptionCode) {
		case 1:
			cout << "[Error] Could not open the input file \"" << fileNameHTML
					<< "\"\n";
			break;
		case 3:
			cout << "[Error] Could not close the input file \"" << fileNameHTML
					<< "\"\n";
			break;
		}
	}

	cout << "Enter any key to exit...\n";
	string temp;
	getline(cin, temp);
	return 0;
}
