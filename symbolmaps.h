#pragma once

#include "common.h"

#include <LibLsp/lsp/lsPosition.h>

#include <unordered_map>
#include <unordered_set>
#include <tuple>
#include <utility>

namespace AsymptoteLsp
{
  struct SymbolLit
  {
    std::string name;
    std::vector<std::string> scopes;

    SymbolLit(std::string symName) :
      name(std::move(symName))
    {
    }

    SymbolLit(std::string symName, std::vector<std::string> scope) :
            name(std::move(symName)), scopes(std::move(scope))
    {
    }

    ~SymbolLit() = default;

    SymbolLit(SymbolLit const& sym) :
      name(sym.name), scopes(sym.scopes)
    {
    }

    SymbolLit& operator=(SymbolLit const& sym)
    {
      name = sym.name;
      scopes = sym.scopes;
      return *this;
    }

    SymbolLit(SymbolLit&& sym) noexcept :
      name(std::move(sym.name)), scopes(std::move(sym.scopes))
    {
    }

    SymbolLit& operator=(SymbolLit&& sym) noexcept
    {
      name = std::move(sym.name);
      scopes = std::move(sym.scopes);
      return *this;
    }

    bool operator==(SymbolLit const& other) const
    {
      return name == other.name and scopes == other.scopes;
    }

    bool matchesRaw(std::string const& sym) const
    {
      return name == sym;
    }
  };
} // namespace AsymptoteLsp

namespace std
{
  using AsymptoteLsp::SymbolLit;

  template<>
  struct hash<SymbolLit>
  {
    std::size_t operator()(SymbolLit const& sym) const
    {
      size_t final_hash = 0;
      final_hash ^= hash<std::string>()(sym.name);
      for (auto const& accessor : sym.scopes)
      {
        final_hash = (final_hash << 1) ^ hash<std::string>()(accessor);
      }
      return final_hash;
    }
  };
} // namespace std

namespace AsymptoteLsp
{
  using std::unordered_map;
  struct SymbolContext;

  typedef std::pair<std::string, SymbolContext*> contextedSymbol;
  typedef std::pair<size_t, size_t> posInFile;
  typedef std::pair<std::string, posInFile> filePos;
  typedef std::tuple<std::string, posInFile, posInFile> posRangeInFile;
  typedef std::tuple<SymbolLit, posInFile, posInFile> fullSymPosRangeInFile;


  // NOTE: lsPosition is zero-indexed, while all Asymptote positions (incl this struct) is 1-indexed.
  inline posInFile fromLsPosition(lsPosition const& inPos)
  {
    return std::make_pair(inPos.line + 1, inPos.character + 1);
  }

  inline lsPosition toLsPosition(posInFile const& inPos)
  {
    return lsPosition(inPos.first - 1, inPos.second - 1);
  }

  inline bool posLt(posInFile const& p1, posInFile const& p2)
  {
    return (p1.first < p2.first) or ((p1.first == p2.first) and (p1.second < p2.second));
  }

  std::string getPlainFile();

  // filename to positions
  struct positions
  {
    std::unordered_map<std::string, std::vector<posInFile>> pos;

    positions() = default;
    explicit positions(filePos const& positionInFile);
    void add(filePos const& positionInFile);
  };


  struct SymbolInfo
  {
    std::string name;
    optional<std::string> type;
    posInFile pos;
    // std::optional<size_t> array_dim;

    SymbolInfo() : type(nullopt), pos(1, 1) {}

    SymbolInfo(std::string inName, posInFile position):
      name(std::move(inName)), type(nullopt), pos(std::move(position)) {}

    SymbolInfo(std::string inName, std::string inType, posInFile position):
      name(std::move(inName)), type(std::move(inType)), pos(std::move(position)) {}

    SymbolInfo(SymbolInfo const& symInfo) = default;

    SymbolInfo& operator=(SymbolInfo const& symInfo) noexcept = default;

    SymbolInfo(SymbolInfo&& symInfo) noexcept :
            name(std::move(symInfo.name)), type(std::move(symInfo.type)), pos(std::move(symInfo.pos))
    {
    }

    SymbolInfo& operator=(SymbolInfo&& symInfo) noexcept
    {
      name = std::move(symInfo.name);
      type = std::move(symInfo.type);
      pos = std::move(symInfo.pos);
      return *this;
    }

    virtual ~SymbolInfo() = default;

    bool operator==(SymbolInfo const& sym) const;

    [[nodiscard]]
    virtual std::string signature() const;
  };

  struct FunctionInfo: SymbolInfo
  {
    std::string returnType;
    using typeName = std::pair<std::string, optional<std::string>>;
    std::vector<typeName> arguments;
    optional<typeName> restArgs;

    FunctionInfo(std::string name, posInFile pos, std::string returnTyp):
            SymbolInfo(std::move(name), std::move(pos)),
            returnType(std::move(returnTyp)),
            arguments(), restArgs(nullopt) {}

    ~FunctionInfo() override = default;

    [[nodiscard]]
    std::string signature() const override;
  };


  struct TypeDec
  {
    posInFile position;
    std::string typeName;

    TypeDec(): position(1, 1) {}
    virtual ~TypeDec() = default;

    TypeDec(posInFile pos, std::string typName):
            position(std::move(pos)), typeName(std::move(typName))
    {
    }

    TypeDec(TypeDec const& typDec) = default;
    TypeDec& operator= (TypeDec const& typDec) = default;

    TypeDec(TypeDec&& typDec) noexcept = default;
    TypeDec& operator= (TypeDec&& typDec) = default;

    [[nodiscard]]
    virtual unique_ptr<TypeDec> clone() const
    {
      return make_unique<TypeDec>(*this);
    }
  };

  struct TypedefDec : public TypeDec
  {
    std::string destName;
  };

  struct StructDecs : public TypeDec
  {
    SymbolContext* ctx;

    StructDecs(): TypeDec(), ctx(nullptr) {}
    ~StructDecs() override = default;

    StructDecs(posInFile pos, std::string typName) :
            TypeDec(std::move(pos), std::move(typName)), ctx(nullptr)
    {
    }

    StructDecs(posInFile pos, std::string typName, SymbolContext* ctx) :
            TypeDec(std::move(pos), std::move(typName)), ctx(ctx)
    {
    }

    [[nodiscard]]
    unique_ptr<TypeDec> clone() const override
    {
      return std::unique_ptr<TypeDec>(new StructDecs(*this));
    }
  };

  struct SymbolMaps
  {
    unordered_map <std::string, SymbolInfo> varDec;
    unordered_map <std::string, std::vector<FunctionInfo>> funDec;
    // can refer to other files
    unordered_map <SymbolLit, positions> varUsage;
    unordered_map <std::string, unique_ptr<TypeDec>> typeDecs;

    // python equivalent of dict[str, list[tuple(pos, sym)]]
    // filename -> list[(position, symbol)]

    std::vector<std::pair<posInFile, SymbolLit>> usageByLines;

    SymbolMaps() = default;
    ~SymbolMaps() = default;

    SymbolMaps(SymbolMaps const& symMap) :
    varDec(symMap.varDec), funDec(symMap.funDec), varUsage(symMap.varUsage), typeDecs(),
    usageByLines(symMap.usageByLines)
    {
      for (auto const& [ty, tyDec] : symMap.typeDecs)
      {
          typeDecs.emplace(ty, tyDec != nullptr ? tyDec->clone() : nullptr);
      }
    }

    SymbolMaps& operator=(SymbolMaps const& symMap)
    {
      varDec = symMap.varDec;
      funDec = symMap.funDec;
      varUsage = symMap.varUsage;
      usageByLines = symMap.usageByLines;

      for (auto const& [ty, tyDec] : symMap.typeDecs)
      {
        typeDecs.emplace(ty, tyDec != nullptr ? tyDec->clone() : nullptr);
      }

      return *this;
    }

    SymbolMaps(SymbolMaps&& symMap) noexcept:
            varDec(std::move(symMap.varDec)), funDec(std::move(symMap.funDec)), varUsage(std::move(symMap.varUsage)),
            typeDecs(std::move(symMap.typeDecs)), usageByLines(std::move(symMap.usageByLines))
    {
    }

    SymbolMaps& operator=(SymbolMaps&& symMap)
    {
      varDec = std::move(symMap.varDec);
      funDec = std::move(symMap.funDec);
      varUsage = std::move(symMap.varUsage);
      usageByLines = std::move(symMap.usageByLines);
      typeDecs = std::move(symMap.typeDecs);

      return *this;
    }

    inline void clear()
    {
      varDec.clear();
      funDec.clear();
      varUsage.clear();
      usageByLines.clear();
      typeDecs.clear();
    }
    optional<fullSymPosRangeInFile> searchSymbol(posInFile const& inputPos);
    FunctionInfo& addFunDef(std::string const& funcName, posInFile const& position, std::string const& returnType);

  private:
    friend ostream& operator<<(std::ostream& os, const SymbolMaps& sym);
  };


  struct SymbolContext
  {
    optional<std::string> fileLoc;
    posInFile contextLoc;
    SymbolContext* parent;
    SymbolMaps symMap;

    // file interactions
    // access -> (file, id)
    // unravel -> id
    // include -> file
    // import = acccess + unravel

    using extRefMap = std::unordered_map<std::string, SymbolContext*>;
    extRefMap extFileRefs;
    std::unordered_map<std::string, std::string> fileIdPair;
    std::unordered_set<std::string> includeVals;
    std::unordered_set<std::string> unravledVals;
    std::vector<std::unique_ptr<SymbolContext>> subContexts;

    SymbolContext():
      parent(nullptr) {
      std::cerr << "created symbol context";
    }

    virtual ~SymbolContext() = default;

    explicit SymbolContext(posInFile loc);
    explicit SymbolContext(posInFile loc, std::string filename);

    SymbolContext(posInFile loc, SymbolContext* contextParent):
      fileLoc(nullopt), contextLoc(std::move(loc)), parent(contextParent)
    {
    }

    template<typename T=SymbolContext, typename=std::enable_if<std::is_base_of<SymbolContext, T>::value>>
    T* newContext(posInFile const& loc)
    {
      auto& newCtx = subContexts.emplace_back(std::make_unique<T>(loc, this));
      return static_cast<T*>(newCtx.get());
    }

    template<typename T=TypeDec, typename=std::enable_if<std::is_base_of<TypeDec, T>::value>>
    T* newTypeDec(std::string const& tyName, posInFile const& loc)
    {
      auto [it, succ] = symMap.typeDecs.emplace(tyName, std::make_unique<T>(loc, tyName));
      return succ ? static_cast<T*>(it->second.get()) : static_cast<T*>(nullptr);
    }

    SymbolContext(SymbolContext const& symCtx) :
      fileLoc(symCtx.fileLoc), contextLoc(symCtx.contextLoc),
      parent(symCtx.parent), symMap(symCtx.symMap),
      extFileRefs(symCtx.extFileRefs), fileIdPair(symCtx.fileIdPair),
      includeVals(symCtx.includeVals)
    {
      for (auto& ctx : symCtx.subContexts)
      {
        subContexts.push_back(make_unique<SymbolContext>(*ctx));
      }
    }

    SymbolContext& operator= (SymbolContext const& symCtx)
    {
      fileLoc = symCtx.fileLoc;
      contextLoc = symCtx.contextLoc;
      parent = symCtx.parent;
      symMap = symCtx.symMap;
      extFileRefs = symCtx.extFileRefs;
      fileIdPair = symCtx.fileIdPair;
      includeVals = symCtx.includeVals;

      for (auto& ctx : symCtx.subContexts)
      {
        subContexts.push_back(make_unique<SymbolContext>(*ctx));
      }

      return *this;
    }

    SymbolContext(SymbolContext&& symCtx) noexcept :
            fileLoc(std::move(symCtx.fileLoc)), contextLoc(std::move(symCtx.contextLoc)),
            parent(symCtx.parent), symMap(std::move(symCtx.symMap)),
            extFileRefs(std::move(symCtx.extFileRefs)), fileIdPair(std::move(symCtx.fileIdPair)),
            includeVals(std::move(symCtx.includeVals)), subContexts(std::move(symCtx.subContexts))
    {
    }

    SymbolContext& operator= (SymbolContext&& symCtx) noexcept
    {
      fileLoc = std::move(symCtx.fileLoc);
      contextLoc = std::move(symCtx.contextLoc);
      parent = symCtx.parent;
      symMap = std::move(symCtx.symMap);
      extFileRefs = std::move(symCtx.extFileRefs);
      fileIdPair = std::move(symCtx.fileIdPair);
      includeVals = std::move(symCtx.includeVals);
      subContexts = std::move(symCtx.subContexts);
      return *this;
    }

    // [file, start, end]
    virtual std::pair<optional<fullSymPosRangeInFile>, SymbolContext*> searchSymbol(posInFile const& inputPos);

    optional<posRangeInFile> searchVarDeclFull(std::string const& symbol,
                                                    optional<posInFile> const& position=nullopt);

    optional<posRangeInFile> searchVarDecl(std::string const& symbol);
    virtual optional<posRangeInFile> searchVarDecl(
            std::string const& symbol, optional<posInFile> const& position);

    // variable signatures
    virtual optional<std::string> searchVarSignature(std::string const& symbol) const;
    virtual optional<std::string> searchVarSignatureFull(std::string const& symbol);
    virtual std::list<std::string> searchFuncSignature(std::string const& symbol);
    virtual std::list<std::string> searchFuncSignatureFull(std::string const& symbol);

    optional<std::string> searchLitSignature(SymbolLit const& symbol);
    optional<posRangeInFile> searchLitPosition(
            SymbolLit const& symbol, optional<posInFile> const& position=nullopt);
    optional<std::string> searchVarType(std::string const& symbol) const;

    virtual std::list<extRefMap::iterator> getEmptyRefs();

    optional<std::string> getFileName() const;

    SymbolContext* getParent()
    {
      return parent == nullptr ? this : parent->getParent();
    }

    bool addEmptyExtRef(std::string const& fileName)
    {
      auto [it, success] = extFileRefs.emplace(fileName, nullptr);
      return success;
    }

    void reset(std::string const& newFile)
    {
      fileLoc = newFile;
      contextLoc = std::make_pair(1,1);
      clear();
    }

    void clear()
    {
      parent = nullptr;
      symMap.clear();
      clearExtRefs();
      subContexts.clear();
    }

    void clearExtRefs()
    {
      extFileRefs.clear();
      fileIdPair.clear();
      includeVals.clear();
      unravledVals.clear();
    }

  protected:
    using SymCtxSet = std::unordered_set<SymbolContext*>;
    template<typename TRet, typename TFn>
    optional<TRet> _searchVarFull(std::unordered_set<SymbolContext*>& searched, TFn const& fnLocalPredicate)
    {
      auto [it, notSearched] = searched.emplace(this->getParent());
      if (not notSearched)
      {
        // a loop in the search path. Stop now.
        return nullopt;
      }

      // local search first
      optional<TRet> returnVal=fnLocalPredicate(this);
      return returnVal.has_value() ? returnVal : searchVarExt<TRet, TFn>(searched, fnLocalPredicate);
    }

    template<typename TRet, typename TFn>
    optional<TRet> searchVarExt(std::unordered_set<SymbolContext*>& searched, TFn const& fnLocalPredicate)
    {
      std::unordered_set<std::string> traverseSet(unravledVals);
      traverseSet.insert(includeVals.begin(), includeVals.end());

      for (auto const& traverseVal : traverseSet)
      {
        if (traverseVal == this->getFileName())
        {
          continue;
        }
        auto returnValF = extFileRefs.at(traverseVal)->_searchVarFull<TRet, TFn>(searched, fnLocalPredicate);
        if (returnValF.has_value())
        {
          return returnValF;
        }
      }
      return nullopt;
    }

    virtual std::list<std::string> _searchFuncSignatureFull(std::string const& symbol, SymCtxSet& searched);
    virtual std::list<std::string> searchFuncSignatureExt(std::string const& symbol, SymCtxSet& searched);
    void addPlainFile();
  };

  struct AddDeclContexts: SymbolContext
  {
    unordered_map <std::string, SymbolInfo> additionalDecs;
    AddDeclContexts(): SymbolContext() {}

    explicit AddDeclContexts(posInFile loc):
      SymbolContext(loc) {}

    AddDeclContexts(posInFile loc, SymbolContext* contextParent):
      SymbolContext(loc, contextParent) {}

    ~AddDeclContexts() override = default;

    optional<posRangeInFile> searchVarDecl(std::string const& symbol,
                                                optional<posInFile> const& position) override;
    optional<std::string> searchVarSignature(std::string const& symbol) const override;
  };
}