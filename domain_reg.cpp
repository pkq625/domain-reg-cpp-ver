#include <cstdio>
#include <vector>
#include <numeric>
#include <sstream>

#include <iostream>
#include <map>
#include <utility>
#include <unordered_map>

enum LEVELS{
    LEVEL_0,
    LEVEL_1,
    LEVEL_2,
    LEVEL_3,
};
class TrieNode{
public:
    TrieNode()= default;
    TrieNode(char  item):item(item), init_item(item){}
    /*当前字符串，如果要支持中文，得改成wchar_t*/
    char init_item;
    char item;
    /* 比如：{2，5}表示这个重复的次数2<= n <=5 */
    int repNumLow = 1; // repeat num lower bound
    int repNumUpp = 1; // repeat num upper bound
    /*当前级别*/
    int itemLevel = 0;
    /*用来合并和剪枝*/
    int inNum = 0; // 以该节点开始的有多少个字符串
    int endNum = 0; // 以该节点结束的有多少个字符串
    /* 记录当前节点的所有子节点 */
    std::unordered_map<char, TrieNode*>childNodes;
public:
    // 一些getter和setter
    char getItem() const {return this->item;}
    TrieNode* setItem(const char itemData) {this->item = itemData; return this;}

    int getRepNumLow() const {return this->repNumLow;};
    TrieNode* setRepNumLow(int l) {this->repNumLow = l; return this;}

    int getRepNumUpp() const {return this->repNumUpp;};
    TrieNode* setRepNumUpp(int u) {this->repNumUpp = u; return this;}
    TrieNode* incRepNumUpp(int num) {this->repNumUpp += num; return this;}

    int getItemLevel() const {return this->itemLevel;};
    TrieNode* setItemLevel(int l) {this->itemLevel = l; return this;}
    TrieNode* incItemLevel() {this->itemLevel++; return this;}

    int getInNum() const {return this->inNum;};
    TrieNode* setInNum(int num) {this->inNum = num; return this;}
    TrieNode* incInNum(int num) {this->inNum+=num; return this;}
    TrieNode* decInNum(int num) {this->inNum-=num; return this;}

    int getEndNum() const {return this->endNum;};
    TrieNode* setEndNum(int num) {this->endNum = num; return this;}
    TrieNode* incEndNum(int num) {this->endNum+=num; return this;}
    TrieNode* decEndNum(int num) {this->endNum-=num; return this;}

    bool hasChildren() const {return !childNodes.empty();}
    TrieNode* getChild(const char itemData) {auto it = this->childNodes.find(itemData); return it != childNodes.end()?it->second: nullptr;}
    const std::unordered_map<char, TrieNode*>& getChildNodes() const {return this->childNodes;}
    TrieNode* setChildNodes(const std::unordered_map<char, TrieNode*>& children){
        this->childNodes.clear();
        this->childNodes = children;
        return this;
    }
    TrieNode* clearChildNodes(const std::unordered_map<char, TrieNode*>& children){
        this->childNodes.clear();
        return this;
    }
    TrieNode* addChild(const char itemData, int repNum){
        auto it = childNodes.find(itemData);
        TrieNode* child = it != childNodes.end() ? it->second : new TrieNode(itemData);
        child->incInNum(repNum);
        if (it == childNodes.end()) childNodes[itemData] = child;
        return child;
    }
    // 一些特殊操作
    TrieNode* copy(TrieNode* node){
        this->item = node->item;
        this->repNumLow = node->repNumLow;
        this->repNumUpp = node->repNumUpp;
        this->inNum = node->inNum;
        this->endNum = node->endNum;
        this->itemLevel = node->itemLevel;
        this->childNodes.clear();
        this->childNodes = node->childNodes;
        return this;
    }
    TrieNode* incRepNums(TrieNode* node){
        this->repNumUpp += node->repNumUpp;
        this->repNumLow += node->repNumLow;
        return this;
    }
    // 合并操作
    TrieNode* mergeRepNum(TrieNode* node){
        if (this->repNumLow > node->repNumLow) this->repNumLow = node->repNumLow;
        if (this->repNumUpp < node->repNumUpp) this->repNumUpp = node->repNumUpp;
        return this;
    }
    TrieNode* mergeChildNodes(TrieNode* other, bool fromDeep=false) {
        for (auto& [_, otherChild] : other->getChildNodes()) {
            if (this->childNodes.find(otherChild->getItem()) != this->childNodes.end()) {
                this->childNodes[otherChild->getItem()]->mergeNode(otherChild);
            } else {
                this->childNodes[otherChild->getItem()] = otherChild;
            }
        }
        return this;
    }
    TrieNode* mergeNode(TrieNode* node){
        this->incInNum(node->getInNum())
                ->incEndNum(node->getEndNum())
                ->mergeRepNum(node)
                ->mergeChildNodes(node);
        return this;
    }
    void printNodeProperties(const std::string &ind){
        std::cout<<ind<<"Item: " << this->item << std::endl;
        std::cout<<ind<<"Repeat Num Lower bound: " << this->repNumLow << std::endl;
        std::cout<<ind<<"Repeat Num Upper bound: " << this->repNumUpp << std::endl;
        std::cout<<ind<<"Item Level " << this->itemLevel << std::endl;
        std::cout<<ind<<"Number of Items Into This Node: " << this->inNum << std::endl;
        std::cout<<ind<<"Number of Items End From This Node: " << this->endNum << std::endl;
        std::cout<<ind<<"Has Children: " << this->hasChildren() << std::endl;
        for (auto & childNode : this->childNodes) {
            std::cout << ind << "Child Node " << childNode.first << ": " << std::endl;
            childNode.second->printNodeProperties(ind + "    ");
        }
    }
};
// 可以通过改这个，把IP之类的信息作为新的一层
enum CHAR_PATTERNS{
    CHAR_ALL, // .
    CHAR_WORD, // digital + word + _
    LETTER, // a-z A-Z \c
    LETTER_U, // A-Z \L
    LETTER_L, // a-z \l
    DIGITAL, // 0-9 \d
    OTHER_LANG, // other language
    BLANK, // \s
};
const int SPECIALCHAR = BLANK;
class RegTree{
private:
    int maxLevelUp = 3; // 
    int curLevel = 0; // 这两个和enum搭配使用
    const char rootId = 65535;
    bool pruneDirtyData = true;
    double pruneScore = 0.2; // 调参
    /**
     * 若某个分支的数据量占比大于此值，则不对该分支进行升级，如此可保留较大比重数据的一些细节信息
     *   1.0 表示关闭此功能
     */
    double absGoodBranch = 0.3; // 调参
    int absGoodBranchNum = 0;
    TrieNode* ROOT;
    bool mergeDepth = false;
    std::unordered_map<int, int> lengthInfo;
    const int BRANCH_NUM = 3;

    const std::vector<char> LEVELUPTABLE = {
            BLANK, BLANK, BLANK, BLANK, BLANK, BLANK, BLANK, BLANK, BLANK, BLANK, BLANK,
            BLANK, BLANK, BLANK, BLANK, BLANK, BLANK, BLANK, BLANK, BLANK, BLANK, BLANK,
            BLANK, BLANK, BLANK, BLANK, BLANK, BLANK, BLANK, BLANK, BLANK, BLANK, BLANK,
            33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
            DIGITAL, DIGITAL, DIGITAL, DIGITAL, DIGITAL, DIGITAL, DIGITAL, DIGITAL, DIGITAL, DIGITAL,
            58, 59, 60, 61, 62, 63, 64, LETTER_U, LETTER_U, LETTER_U, LETTER_U, LETTER_U,
            LETTER_U, LETTER_U, LETTER_U, LETTER_U, LETTER_U, LETTER_U, LETTER_U, LETTER_U,
            LETTER_U, LETTER_U, LETTER_U, LETTER_U, LETTER_U, LETTER_U, LETTER_U, LETTER_U,
            LETTER_U, LETTER_U, LETTER_U, LETTER_U, LETTER_U, 91, 92, 93, 94, 95, 96,
            LETTER_L, LETTER_L, LETTER_L, LETTER_L, LETTER_L, LETTER_L, LETTER_L, LETTER_L,
            LETTER_L, LETTER_L, LETTER_L, LETTER_L, LETTER_L, LETTER_L, LETTER_L, LETTER_L,
            LETTER_L, LETTER_L, LETTER_L, LETTER_L, LETTER_L, LETTER_L, LETTER_L, LETTER_L,
            LETTER_L, LETTER_L, 123, 124, 125, 126, 127
    };

    std::unordered_map<char, std::string> RegMap = {
            {DIGITAL, "\\d"}, {LETTER_L, "[a-z]"},
            {LETTER_U, "[A-Z]"}, {LETTER, "[a-zA-z]"}, {OTHER_LANG, "[\\u0100-\\uffff]"},
            {BLANK, "\\s"}, {CHAR_WORD, "\\w"}, {CHAR_ALL, "."}
    };
public:
    RegTree() {
        ROOT = new TrieNode(rootId);
        ROOT->setItemLevel(99);
    }
    ~RegTree(){delete ROOT;}
    RegTree* addData(const std::string& item, int repeat_times=1){
        if (item.empty()) return this;
        lengthInfo[item.length()] += repeat_times;
        TrieNode* curNode = ROOT;
        curNode->incInNum(repeat_times);
        for(char c: item){
            curNode = curNode->addChild(c, repeat_times);
        }
        curNode->incEndNum(repeat_times);
        return this;
    }
    RegTree* addData(const std::vector<std::string>& dataList){
        for (const auto& data : dataList) {
            addData(data, 1);
        }
        return this;
    }
    // 剪枝
    void prune(TrieNode* node){
        // 对node进行剪枝操作，如果它的其中一个子节点的阈值小于某个值，则减去该子节点；如果它的结束节点小于某个阈值，删去该结束节点
        if (!pruneDirtyData || !node->hasChildren()) return;
        auto childrenNodes = node->getChildNodes();
        int totalSize = (node->getEndNum() > 0)?childrenNodes.size()+1:childrenNodes.size();
        int minSize = node->getInNum() / totalSize * pruneScore;
        if (node->getEndNum() < minSize) node->setEndNum(0);
        std::vector<char> pruneChilds;
        for (const auto& [ch, child] : childrenNodes) {
            if (child->getInNum() < minSize) pruneChilds.push_back(ch);
        }
        for (char ch : pruneChilds) {
            childrenNodes.erase(ch);
        }
        node->setChildNodes(childrenNodes);
    }

    // 升级节点，升级到curLevel
    char charLevelUp(char c, int itemLevel){
        if (itemLevel >= curLevel) return c;
        char upC;
        switch (itemLevel) {
            case 0:
                upC = (c>127)?OTHER_LANG:LEVELUPTABLE[c];
                break;
            case 1:
                upC = (c == DIGITAL || c == LETTER_L || c == LETTER_U || c == '_') ? CHAR_WORD : c;
                break;
            default:
                upC = CHAR_ALL;
                break;
        }
        return (itemLevel < curLevel - 1)? charLevelUp(upC, itemLevel + 1):upC;
    }
    int levelUpAndMerge(TrieNode* node, bool forceLu= false){
        int lvNum = 0;
        int absGoodNum = static_cast<int>(node->getInNum() * absGoodBranch);
        std::unordered_map<char, TrieNode*> newChilds;

        for (const auto& [_, child] : node->getChildNodes()) {
            if (forceLu || (child->getInNum() < absGoodNum)) {
                lvNum++;
                char itemUp = charLevelUp(child->getItem(), child->getItemLevel());
                child->setItemLevel(curLevel)->setItem(itemUp);
                lvNum += levelUpAndMerge(child, true);
                auto it = newChilds.find(itemUp);
                if (it != newChilds.end()) {
                    it->second->mergeNode(child);
                } else {
                    newChilds[itemUp] = child;
                }
            } else {
                lvNum += levelUpAndMerge(child, false);
                newChilds[child->getItem()] = child;
            }
        }
        node->setChildNodes(newChilds);
        prune(node);
        return lvNum;
    }

    // 深度合并
    void checkLengthInfo() {
        // 深度合并
        mergeDepth = false;
        if (lengthInfo.empty()) return;
        int maxGroups = static_cast<int>(1 / (absGoodBranch + 0.000001)) + 1;
        // 计算分支平均长度
        int avg = ROOT->getInNum() / lengthInfo.size() / 10;
        int groups = 0;
        for (const auto& [_, supp] : lengthInfo) {
            if (supp >= avg) groups++;
        }
        if (groups > maxGroups) mergeDepth = true;
    }
    int DeepMerge(TrieNode* node){
        int mNum = 0;
        if (!node->hasChildren()) return 0;
        auto childs = node->getChildNodes();
        for (const auto& [_, child] : childs) {
            mNum += DeepMerge(child);
        }
        prune(node);
        auto it = childs.find(node->getItem());
        if (it != childs.end()) {
            TrieNode* child = it->second;
            if (childs.size() == 1 && node->getEndNum() == 0) {
                mNum++;
                node->setChildNodes(child->getChildNodes())
                        ->incRepNums(child)
                        ->setInNum(child->getInNum())
                        ->setEndNum(child->getEndNum());
            } else if (mergeDepth) {
                mNum++;
                childs.erase(it);
                node->setChildNodes(childs);
                node->mergeChildNodes(child, true)
                        ->incRepNumUpp(child->getRepNumUpp())
                        ->incEndNum(child->getEndNum());
                prune(node);
            }
        }
        return mNum;
    }
    // 开始构建Reg
    void mine() {
        checkLengthInfo();
        absGoodBranchNum = static_cast<int>(ROOT->getInNum() * absGoodBranch);
        int changedNum = 0;
        do {
            changedNum = levelUpAndMerge(ROOT);
            changedNum += DeepMerge(ROOT);
            if (changedNum > 0) {
                std::string reg = getRegString();
                std::cout << reg << std::endl;
            }
            curLevel++;
        } while (curLevel <= maxLevelUp && changedNum > 0);
    }
    std::string getRegString() { return convertTree2String(ROOT, true); }

    std::string convertTree2String(TrieNode* node, bool bHead) {
        if (!node->hasChildren()) return "";
        std::vector<std::string> childRegs;
        for (const auto& [_, child] : node->getChildNodes()) {
            childRegs.push_back(convert1Node(child));
        }
        if (childRegs.size() == 1) return childRegs[0];
        std::string reg;
        if (bHead) {
            for (size_t i = 0; i < childRegs.size(); ++i) {
                reg += childRegs[i];
                if (i != childRegs.size() - 1) reg += "|";
            }
        } else {
            reg += "(?:";
            for (size_t i = 0; i < childRegs.size()-1; ++i) {
                reg += childRegs[i];
                reg += "|";
            }
            reg += childRegs[childRegs.size() - 1];
            reg +=  ")";
        }
        return reg;
    }
    // aaaaa -> a{5}
    std::string convert1Node(TrieNode* node) {
        char item = node->getItem();
        std::string itemReg = getItemChar(item);
        std::string reg = itemReg;
        int rl = node->getRepNumLow(), ru = node->getRepNumUpp();
        if (rl == ru) {
            if (rl > 1) {
                if (rl < 4 && item < 128 && item > SPECIALCHAR) {
                    int t = rl - 1;
                    while (t--)
                        reg += std::string(itemReg);
                } else {
                    reg += "{" + std::to_string(rl) + "}";
                }
            }
        } else {
            reg += "{" + std::to_string(rl) + "," + std::to_string(ru) + "}";
        }
        if (!node->hasChildren()) return reg;
        std::string childReg = convertTree2String(node, false);
        if (childReg.empty()) return reg;
        reg += (node->getEndNum() > 0) ? "(?:" + childReg + ")?" : childReg;
        return reg;
    }

    std::string getItemChar(char c) {
        auto it = RegMap.find(c);
        return (it != RegMap.end()) ? it->second : std::string(1, c);
    }
};
std::vector<std::string> stringSplit(const std::string& str, char delim) {
    std::stringstream ss(str);
    std::string item;
    std::vector<std::string> elems;
    while (std::getline(ss, item, delim)) {
        if (!item.empty()) {
            //std::cout<<item<<std::endl;
            elems.push_back(item);
        }
    }
    return elems;
}
#include <fstream>
//#include <regex>
int totLine = 0;
int totData = 0;
void test4(std::string filename){
    RegTree reg;
    
    std::vector<std::string> lines;
    std::string line;
    std::ifstream input_file(filename);
    if (!input_file.is_open()) {
        return;
    }
    //static const std::regex integer_regex("[+-]?[0-9]+");
    while (getline(input_file, line)) {
        std::vector<std::string> s = stringSplit(line, ',');
        if (s.size() != 2) {std::cout<<line<<std::endl;continue;}
        int n;
        try{n = stoi(s[1]);}
        catch (...){std::cout<<line<<std::endl;continue;}
        totData += n;
        if (s[0].size() < 20){continue;}
        totLine += n;
        //if (n < 100) continue;
        reg.addData(s[0], n);
    }
    input_file.close();

    reg.mine();
    std::cout << std::endl;
    std::cout << reg.getRegString() << std::endl;
    std::cout << std::endl;
    std::cout << totLine << std::endl;
    std::cout << std::endl;
    std::cout << totData << std::endl;
}
int main(int ac, char* av[]){
    test4(av[1]);
    return 0;
}
