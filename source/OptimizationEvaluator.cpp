#pragma warning(disable:4996)
#include <functional>

#include <Pita/OptimizationEvaluator.hpp>
#include <Pita/Evaluator.hpp>
#include <Pita/BinaryEvaluator.hpp>

namespace cgl
{
	boost::optional<size_t> SatVariableBinder::freeVariableIndex(Address reference)
	{
		for (size_t i = 0; i < freeVariables.size(); ++i)
		{
			if (freeVariables[i] == reference)
			{
				return i;
			}
		}

		return boost::none;
	}

	//Address -> 参照ID
	boost::optional<int> SatVariableBinder::addSatRef(Address reference)
	{
		//以前に出現して登録済みのfree変数はそのまま返す
		auto refID_It = invRefs.find(reference);
		if (refID_It != invRefs.end())
		{
			//CGL_DebugLog("addSatRef: 登録済み");
			return refID_It->second;
		}

		//初めて出現したfree変数は登録してから返す
		if (auto indexOpt = freeVariableIndex(reference))
		{
			const int referenceID = refs.size();
			usedInSat[indexOpt.value()] = 1;
			invRefs[reference] = referenceID;
			refs.push_back(reference);

			{
				pEnv->printContext(true);
			}
				
			return referenceID;
		}

		return boost::none;
	}

	bool SatVariableBinder::operator()(const LRValue& node)
	{
		if (node.isLValue())
		{
			const Address address = node.address();

			if (!address.isValid())
			{
				CGL_Error("識別子が定義されていません");
			}

			//free変数にあった場合は制約用の参照値を追加する
			return static_cast<bool>(addSatRef(address));
		}

		return false;
	}

	bool SatVariableBinder::operator()(const Identifier& node)
	{
		Address address = pEnv->findAddress(node);
		if (!address.isValid())
		{
			//CGL_Error("識別子が定義されていません");
			//この中で作られた変数だった場合、定義されていない可能性がある
			return false;
		}

		//free変数にあった場合は制約用の参照値を追加する
		return static_cast<bool>(addSatRef(address));
	}

	bool SatVariableBinder::operator()(const UnaryExpr& node)
	{
		return boost::apply_visitor(*this, node.lhs);
	}

	bool SatVariableBinder::operator()(const BinaryExpr& node)
	{
		const bool a = boost::apply_visitor(*this, node.lhs);
		const bool b = boost::apply_visitor(*this, node.rhs);
		return a || b;
	}
	
	bool SatVariableBinder::callFunction(const FuncVal& funcVal, const std::vector<Address>& expandedArguments)
	{
		/*FuncVal funcVal;

		if (auto opt = AsOpt<FuncVal>(node.funcRef))
		{
			funcVal = opt.value();
		}
		else
		{
			const Address funcAddress = pEnv->findAddress(As<Identifier>(node.funcRef));
			if (funcAddress.isValid())
			{
				if (auto funcOpt = pEnv->expandOpt(funcAddress))
				{
					if (IsType<FuncVal>(funcOpt.value()))
					{
						funcVal = As<FuncVal>(funcOpt.value());
					}
					else
					{
						CGL_Error("指定された変数名に紐つけられた値が関数でない");
					}
				}
				else
				{
					CGL_Error("ここは通らないはず");
				}
			}
			else
			{
				CGL_Error("指定された変数名に値が紐つけられていない");
			}
		}*/

		//組み込み関数の場合は関数定義の中身と照らし合わせるという事ができないため、とりあえず引数から辿れる要素を全て指定する
		if (funcVal.builtinFuncAddress)
		{
			bool result = false;
			for (Address argument : expandedArguments)
			{
				const auto addresses = pEnv->expandReferences(argument);
				for (Address address : addresses)
				{
					result = static_cast<bool>(addSatRef(address)) || result;
				}
			}

			//もし組み込み関数の引数に変数が指定されていた場合は不連続関数かどうかを保存し、これによって最適化手法を切り替えられるようにしておく
			if (result)
			{
				hasPlateausFunction = pEnv->isPlateausBuiltInFunction(funcVal.builtinFuncAddress.value()) || hasPlateausFunction;
			}

			return result;
		}

		/*std::vector<Address> expandedArguments(node.actualArguments.size());
		for (size_t i = 0; i < expandedArguments.size(); ++i)
		{
			expandedArguments[i] = pEnv->makeTemporaryValue(node.actualArguments[i]);
		}*/

		if (funcVal.arguments.size() != expandedArguments.size())
		{
			CGL_Error("仮引数の数と実引数の数が合っていない");
		}

		pEnv->switchFrontScope();
		pEnv->enterScope();

		for (size_t i = 0; i < funcVal.arguments.size(); ++i)
		{
			pEnv->bindValueID(funcVal.arguments[i], expandedArguments[i]);
		}
		const bool result = boost::apply_visitor(*this, funcVal.expr);

		pEnv->exitScope();
		pEnv->switchBackScope();

		return result;
	}

	bool SatVariableBinder::operator()(const Lines& node)
	{
		bool result = false;
		for (const auto& expr : node.exprs)
		{
			result = boost::apply_visitor(*this, expr) || result;
		}
		return result;
	}

	bool SatVariableBinder::operator()(const If& node)
	{
		bool result = false;
		result = boost::apply_visitor(*this, node.cond_expr) || result;
		result = boost::apply_visitor(*this, node.then_expr) || result;
		if (node.else_expr)
		{
			result = boost::apply_visitor(*this, node.else_expr.value()) || result;
		}
		return result;
	}

	bool SatVariableBinder::operator()(const For& node)
	{
		bool result = false;
			
		//for式の範囲を制約で制御できるようにする意味はあるか？
		//result = boost::apply_visitor(*this, node.rangeEnd) || result;
		//result = boost::apply_visitor(*this, node.rangeStart) || result;

		result = boost::apply_visitor(*this, node.doExpr) || result;
		return result;
	}

	bool SatVariableBinder::operator()(const ListConstractor& node)
	{
		bool result = false;
		for (const auto& expr : node.data)
		{
			result = boost::apply_visitor(*this, expr) || result;
		}
		return result;
	}

	bool SatVariableBinder::operator()(const KeyExpr& node)
	{
		return boost::apply_visitor(*this, node.expr);
	}

	bool SatVariableBinder::operator()(const RecordConstractor& node)
	{
		bool result = false;
		//for (const auto& expr : node.exprs)
		for (size_t i = 0; i < node.exprs.size(); ++i)
		{
			const auto& expr = node.exprs[i];
			//CGL_DebugLog(std::string("BindRecordExpr(") + ToS(i) + ")");
			//printExpr(expr);
			result = boost::apply_visitor(*this, expr) || result;
		}
		return result;
	}

	bool SatVariableBinder::operator()(const Accessor& node)
	{
		CGL_DebugLog("SatVariableBinder::operator()(const Accessor& node)");
		{
			Expr expr = node;
			printExpr(expr);
		}

		Address headAddress;
		const Expr& head = node.head;

		//headがsat式中のローカル変数
		if (IsType<Identifier>(head))
		{
			Address address = pEnv->findAddress(As<Identifier>(head));
			if (!address.isValid())
			{
				CGL_Error("識別子が定義されていません");
			}

			//headは必ず Record/List/FuncVal のどれかであり、double型であることはあり得ない。
			//したがって、free変数にあるかどうかは考慮せず（free変数は冗長に指定できるのであったとしても別にエラーにはしない）、
			//直接Addressとして展開する
			headAddress = address;
		}
		//headがアドレス値
		else if (IsType<LRValue>(head))
		{
			const LRValue& headAddressValue = As<LRValue>(head);
			if (!headAddressValue.isLValue())
			{
				CGL_Error("sat式中のアクセッサの先頭部が不正な値です");
			}

			headAddress = headAddressValue.address();
		}
		else
		{
			CGL_Error("sat中のアクセッサの先頭部に単一の識別子以外の式を用いることはできません");
		}

		Eval evaluator(pEnv);

		Accessor result;

		//isDeterministicがtrueであれば、Evalしないとわからないところまで見に行く
		//isDeterministicがfalseの時は、評価するまでアドレスが不定なので関数の中身までは見に行かない
		bool isDeterministic = true;

		for (const auto& access : node.accesses)
		{
			boost::optional<const Evaluated&> objOpt;
			if (isDeterministic)
			{
				objOpt = pEnv->expandOpt(headAddress);
				if (!objOpt)
				{
					CGL_Error("参照エラー");
				}
			}

			if (IsType<ListAccess>(access))
			{
				const ListAccess& listAccess = As<ListAccess>(access);

				isDeterministic = !boost::apply_visitor(*this, listAccess.index) && isDeterministic;

				if (isDeterministic)
				{
					const Evaluated& objRef = objOpt.value();
					if (!IsType<List>(objRef))
					{
						CGL_Error("オブジェクトがリストでない");
					}
					const List& list = As<const List&>(objRef);

					Evaluated indexValue = pEnv->expand(boost::apply_visitor(evaluator, listAccess.index));
					if (auto indexOpt = AsOpt<int>(indexValue))
					{
						headAddress = list.get(indexOpt.value());
					}
					else
					{
						CGL_Error("list[index] の index が int 型でない");
					}
				}
			}
			else if (IsType<RecordAccess>(access))
			{
				const RecordAccess& recordAccess = As<RecordAccess>(access);

				if (isDeterministic)
				{
					const Evaluated& objRef = objOpt.value();
					if (!IsType<Record>(objRef))
					{
						CGL_Error("オブジェクトがレコードでない");
					}
					const Record& record = As<const Record&>(objRef);

					auto it = record.values.find(recordAccess.name);
					if (it == record.values.end())
					{
						CGL_Error("指定された識別子がレコード中に存在しない");
					}

					headAddress = it->second;
				}
			}
			else
			{
				const FunctionAccess& funcAccess = As<FunctionAccess>(access);

				if (isDeterministic)
				{
					//Case2(関数引数がfree)への対応
					for (const auto& argument : funcAccess.actualArguments)
					{
						isDeterministic = !boost::apply_visitor(*this, argument) && isDeterministic;
					}

					//呼ばれる関数の実体はその引数には依存しないため、ここでisDeterministicがfalseになっても問題ない

					//Case4以降への対応は関数の中身を見に行く必要がある
					const Evaluated& objRef = objOpt.value();
					if (!IsType<FuncVal>(objRef))
					{
						CGL_Error("オブジェクトが関数でない");
					}
					const FuncVal& function = As<const FuncVal&>(objRef);

					//Case4,6への対応
					/*
					std::vector<Evaluated> arguments;
					for (const auto& expr : funcAccess.actualArguments)
					{
					//ここでexpandして大丈夫なのか？
					arguments.push_back(pEnv->expand(boost::apply_visitor(evaluator, expr)));
					}
					Expr caller = FunctionCaller(function, arguments);
					isDeterministic = !boost::apply_visitor(*this, caller) && isDeterministic;
					*/
					std::vector<Address> arguments;
					for (const auto& expr : funcAccess.actualArguments)
					{
						const LRValue lrvalue = boost::apply_visitor(evaluator, expr);
						if (lrvalue.isLValue())
						{
							arguments.push_back(lrvalue.address());
						}
						else
						{
							arguments.push_back(pEnv->makeTemporaryValue(lrvalue.evaluated()));
						}
					}
					isDeterministic = !callFunction(function, arguments) && isDeterministic;

					//ここまでで一つもfree変数が出てこなければこの先の中身も見に行く
					if (isDeterministic)
					{
						//const Evaluated returnedValue = pEnv->expand(boost::apply_visitor(evaluator, caller));
						const Evaluated returnedValue = pEnv->expand(evaluator.callFunction(function, arguments));
						headAddress = pEnv->makeTemporaryValue(returnedValue);
					}
				}
			}
		}

		if (isDeterministic)
		{
			return static_cast<bool>(addSatRef(headAddress));
		}

		return true;
	}

	boost::optional<double> EvalSatExpr::expandFreeOpt(Address address)const
	{
		auto it = invRefs.find(address);
		if (it != invRefs.end())
		{
			return data[it->second];
		}
		return boost::none;
	}

	Evaluated EvalSatExpr::operator()(const LRValue& node)
	{
		//CGL_DebugLog("Evaluated operator()(const LRValue& node)");
		if (node.isLValue())
		{
			if (auto opt = expandFreeOpt(node.address()))
			{
				return opt.value();
			}
			//CGL_DebugLog(std::string("address: ") + node.address().toString());
			return pEnv->expand(node.address());
		}
		return node.evaluated();
	}

	Evaluated EvalSatExpr::operator()(const SatReference& node)
	{
		CGL_Error("不正な式");
		return 0;
	}

	Evaluated EvalSatExpr::operator()(const Identifier& node)
	{
		//CGL_DebugLog("Evaluated operator()(const Identifier& node)");
		//pEnv->printContext(true);
		//CGL_DebugLog(std::string("find Identifier(") + std::string(node) + ")");
		const Address address = pEnv->findAddress(node);
		if (auto opt = expandFreeOpt(address))
		{
			return opt.value();
		}
		return pEnv->expand(address);
	}

	Evaluated EvalSatExpr::operator()(const UnaryExpr& node)
	{
		//CGL_DebugLog("Evaluated operator()(const UnaryExpr& node)");
		//if (node.op == UnaryOp::Not)
		{
			CGL_Error("TODO: sat宣言中の単項演算子は未対応です");
		}

		return 0;
	}

	Evaluated EvalSatExpr::operator()(const BinaryExpr& node)
	{
		//CGL_DebugLog("Evaluated operator()(const BinaryExpr& node)");
			
		const Evaluated rhs = boost::apply_visitor(*this, node.rhs);
		if (node.op != BinaryOp::Assign)
		{
			const Evaluated lhs = boost::apply_visitor(*this, node.lhs);

			switch (node.op)
			{
			case BinaryOp::And: return Add(lhs, rhs, *pEnv);
			case BinaryOp::Or:  return Min(lhs, rhs, *pEnv);

			case BinaryOp::Equal:        return Abs(Sub(lhs, rhs, *pEnv), *pEnv);
			case BinaryOp::NotEqual:     return Equal(lhs, rhs, *pEnv) ? 1.0 : 0.0;
			case BinaryOp::LessThan:     return Max(Sub(lhs, rhs, *pEnv), 0.0, *pEnv);
			case BinaryOp::LessEqual:    return Max(Sub(lhs, rhs, *pEnv), 0.0, *pEnv);
			case BinaryOp::GreaterThan:  return Max(Sub(rhs, lhs, *pEnv), 0.0, *pEnv);
			case BinaryOp::GreaterEqual: return Max(Sub(rhs, lhs, *pEnv), 0.0, *pEnv);

			case BinaryOp::Add: return Add(lhs, rhs, *pEnv);
			case BinaryOp::Sub: return Sub(lhs, rhs, *pEnv);
			case BinaryOp::Mul: return Mul(lhs, rhs, *pEnv);
			case BinaryOp::Div: return Div(lhs, rhs, *pEnv);

			case BinaryOp::Pow:    return Pow(lhs, rhs, *pEnv);
			case BinaryOp::Concat: return Concat(lhs, rhs, *pEnv);
			}
		}
		else if (auto valOpt = AsOpt<LRValue>(node.lhs))
		{
			const LRValue& val = valOpt.value();
			if (val.isLValue())
			{
				if (val.address().isValid())
				{
					pEnv->assignToObject(val.address(), rhs);
				}
				else
				{
					CGL_Error("reference error");
				}
			}
			else
			{
				CGL_Error("ここは通らないはず");
			}
		}
		else if (auto valOpt = AsOpt<Identifier>(node.lhs))
		{
			const Identifier& identifier = valOpt.value();

			const Address address = pEnv->findAddress(identifier);
			//変数が存在する：代入式
			if (address.isValid())
			{
				pEnv->assignToObject(address, rhs);
			}
			//変数が存在しない：変数宣言式
			else
			{
				pEnv->bindNewValue(identifier, rhs);
			}

			return rhs;
		}
		else if (auto valOpt = AsOpt<Accessor>(node.lhs))
		{
			Eval evaluator(pEnv);
			const LRValue lhs = boost::apply_visitor(evaluator, node.lhs);
			if (lhs.isLValue())
			{
				Address address = lhs.address();
				if (address.isValid())
				{
					pEnv->assignToObject(address, rhs);
					return rhs;
				}
				else
				{
					CGL_Error("参照エラー");
				}
			}
			else
			{
				CGL_Error("アクセッサの評価結果がアドレスでない");
			}
		}

		CGL_Error("ここは通らないはず");
		return 0;
	}

	Evaluated EvalSatExpr::callFunction(const FuncVal& funcVal, const std::vector<Address>& expandedArguments)
	{
		//CGL_DebugLog("Evaluated operator()(const FunctionCaller& callFunc)");

		/*
		std::vector<Address> expandedArguments(callFunc.actualArguments.size());
		for (size_t i = 0; i < expandedArguments.size(); ++i)
		{
			expandedArguments[i] = pEnv->makeTemporaryValue(callFunc.actualArguments[i]);
		}

		FuncVal funcVal;

		if (auto opt = AsOpt<FuncVal>(callFunc.funcRef))
		{
			funcVal = opt.value();
		}
		else
		{
			const Address funcAddress = pEnv->findAddress(As<Identifier>(callFunc.funcRef));
			if (funcAddress.isValid())
			{
				if (auto funcOpt = pEnv->expandOpt(funcAddress))
				{
					if (IsType<FuncVal>(funcOpt.value()))
					{
						funcVal = As<FuncVal>(funcOpt.value());
					}
					else
					{
						CGL_Error("指定された変数名に紐つけられた値が関数でない");
					}
				}
				else
				{
					CGL_Error("ここは通らないはず");
				}
			}
			else
			{
				CGL_Error("指定された変数名に値が紐つけられていない");
			}
		}
		*/

		if (funcVal.builtinFuncAddress)
		{
			return pEnv->callBuiltInFunction(funcVal.builtinFuncAddress.value(), expandedArguments);
		}

		if (funcVal.arguments.size() != expandedArguments.size())
		{
			CGL_Error("仮引数の数と実引数の数が合っていない");
		}

		//CGL_DebugLog("");

		pEnv->switchFrontScope();

		//CGL_DebugLog("");

		pEnv->enterScope();

		//CGL_DebugLog("");

		for (size_t i = 0; i < funcVal.arguments.size(); ++i)
		{
			pEnv->bindValueID(funcVal.arguments[i], expandedArguments[i]);
			//CGL_DebugLog(std::string("bind: ") + std::string(funcVal.arguments[i]) + " -> Address(" + expandedArguments[i].toString() + ")");
		}

		//CGL_DebugLog("");

		//CGL_DebugLog("Function Definition:");
		//boost::apply_visitor(Printer(), funcVal.expr);

		Evaluated result;
		{
			//関数も通常の関数ではなく、制約を表す関数であるはずなので、評価はEvalではなく*thisで行う

			result = pEnv->expand(boost::apply_visitor(*this, funcVal.expr));
			//CGL_DebugLog("Function Evaluated:");
			//printEvaluated(result, nullptr);
		}
		//Evaluated result = pEnv->expandObject();

		//CGL_DebugLog("");

		//(4)関数を抜ける時に、仮引数は全て解放される
		pEnv->exitScope();

		//CGL_DebugLog("");

		//(5)最後にローカル変数の環境を関数の実行前のものに戻す。
		pEnv->switchBackScope();

		//CGL_DebugLog("");

		return result;
	}

	Evaluated EvalSatExpr::operator()(const Lines& node)
	{
		//CGL_DebugLog("Evaluated operator()(const Lines& node)");
		/*if (node.exprs.size() != 1)
		{
			CGL_Error("不正な式です"); return 0;
		}

		return boost::apply_visitor(*this, node.exprs.front());*/

		pEnv->enterScope();

		Evaluated result;
		for (const auto& expr : node.exprs)
		{
			result = boost::apply_visitor(*this, expr);
		}

		pEnv->exitScope();

		return result;
	}

	Evaluated EvalSatExpr::operator()(const If& if_statement)
	{
		//CGL_DebugLog("Evaluated operator()(const If& if_statement)");
		Eval evaluator(pEnv);

		//if式の条件式は制約が満たされているかどうかを評価するべき
		const Evaluated cond = pEnv->expand(boost::apply_visitor(evaluator, if_statement.cond_expr));
		if (!IsType<bool>(cond))
		{
			CGL_Error("条件は必ずブール値である必要がある");
		}

		//thenとelseは制約が満たされるまでの距離を計算する
		if (As<bool>(cond))
		{
			return pEnv->expand(boost::apply_visitor(*this, if_statement.then_expr));
		}
		else if (if_statement.else_expr)
		{
			return pEnv->expand(boost::apply_visitor(*this, if_statement.else_expr.value()));
		}

		return 0;
	}

	Evaluated EvalSatExpr::operator()(const KeyExpr& node)
	{
		//CGL_DebugLog("Evaluated operator()(const KeyExpr& node)");
		const Evaluated value = boost::apply_visitor(*this, node.expr);
		return KeyValue(node.name, value);
	}

	Evaluated EvalSatExpr::operator()(const RecordConstractor& recordConsractor)
	{
		//CGL_DebugLog("Evaluated operator()(const RecordConstractor& recordConsractor)");
		pEnv->enterScope();
			
		std::vector<Identifier> keyList;
			
		Record record;
		int i = 0;

		for (const auto& expr : recordConsractor.exprs)
		{
			Evaluated value = pEnv->expand(boost::apply_visitor(*this, expr));

			//キーに紐づけられる値はこの後の手続きで更新されるかもしれないので、今は名前だけ控えておいて後で値を参照する
			if (auto keyValOpt = AsOpt<KeyValue>(value))
			{
				const auto keyVal = keyValOpt.value();
				keyList.push_back(keyVal.name);

				//識別子はEvaluatedからはずしたので、識別子に対して直接代入を行うことはできなくなった
				//Assign(ObjectReference(keyVal.name), keyVal.value, *pEnv);

				//したがって、一度代入式を作ってからそれを評価する
					
				/*Expr exprVal = LRValue(keyVal.value);
				Expr expr = BinaryExpr(keyVal.name, exprVal, BinaryOp::Assign);
				boost::apply_visitor(*this, expr);*/

				{
					Address tempVal = pEnv->makeTemporaryValue(keyVal.value);
					pEnv->bindValueID(keyVal.name, tempVal);
					//CGL_DebugLog(std::string("bind: ") + std::string(keyVal.name) + " -> Address(" + tempVal.toString() + ")");
				}
			}

			++i;
		}

		for (const auto& key : keyList)
		{
			//pEnv->printContext(true);
			Address address = pEnv->findAddress(key);
			boost::optional<const Evaluated&> opt = pEnv->expandOpt(address);
			record.append(key, pEnv->makeTemporaryValue(opt.value()));
		}

		pEnv->exitScope();

		return record;
	}

	Evaluated EvalSatExpr::operator()(const Accessor& node)
	{
		/*
		CGL_DebugLog("Evaluated operator()(const Accessor& node)");
		{
			CGL_DebugLog("Access to:");
			Expr expr = node;
			printExpr(expr);
		}
		*/
			
		Address address;

		LRValue headValue = boost::apply_visitor(*this, node.head);
		if (headValue.isLValue())
		{
			address = headValue.address();
		}
		else
		{
			Evaluated evaluated = headValue.evaluated();
			if (auto opt = AsOpt<Record>(evaluated))
			{
				address = pEnv->makeTemporaryValue(opt.value());
			}
			else if (auto opt = AsOpt<List>(evaluated))
			{
				address = pEnv->makeTemporaryValue(opt.value());
			}
			else if (auto opt = AsOpt<FuncVal>(evaluated))
			{
				address = pEnv->makeTemporaryValue(opt.value());
			}
			else
			{
				CGL_Error("アクセッサのヘッドの評価結果が不正");
			}
		}

		for (const auto& access : node.accesses)
		{
			if (auto opt = expandFreeOpt(address))
			{
				//ここで本当はこれに繋がる次のアクセスが存在しないことを確認する必要がある
				return opt.value();
			}

			boost::optional<const Evaluated&> objOpt = pEnv->expandOpt(address);
			if (!objOpt)
			{
				CGL_Error("参照エラー");
			}

			const Evaluated& objRef = objOpt.value();

			if (auto listAccessOpt = AsOpt<ListAccess>(access))
			{
				//CGL_DebugLog("if (auto listAccessOpt = AsOpt<ListAccess>(access))");
				Evaluated value = boost::apply_visitor(*this, listAccessOpt.value().index);

				if (!IsType<List>(objRef))
				{
					CGL_Error("オブジェクトがリストでない");
				}

				const List& list = As<const List&>(objRef);

				const auto clampAddress = [&](int index)->int {return std::max(0, std::min(index, static_cast<int>(list.data.size()) - 1)); };

				if (auto indexOpt = AsOpt<int>(value))
				{
					address = list.get(clampAddress(indexOpt.value()));
				}
				else if (auto indexOpt = AsOpt<double>(value))
				{
					address = list.get(clampAddress(static_cast<int>(std::round(indexOpt.value()))));
				}
				else
				{
					CGL_Error("list[index] の index が数値型でない");
				}
			}
			else if (auto recordAccessOpt = AsOpt<RecordAccess>(access))
			{
				//CGL_DebugLog("else if (auto recordAccessOpt = AsOpt<RecordAccess>(access))");
				if (!IsType<Record>(objRef))
				{
					CGL_Error("オブジェクトがレコードでない");
				}

				const Record& record = As<const Record&>(objRef);
				auto it = record.values.find(recordAccessOpt.value().name);
				if (it == record.values.end())
				{
					CGL_Error("指定された識別子がレコード中に存在しない");
				}

				address = it->second;
			}
			else
			{
				//CGL_DebugLog("auto funcAccess = As<FunctionAccess>(access);");
				auto funcAccess = As<FunctionAccess>(access);

				if (!IsType<FuncVal>(objRef))
				{
					CGL_Error("オブジェクトが関数でない");
				}

				const FuncVal& function = As<const FuncVal&>(objRef);

				/*std::vector<Evaluated> args;
				for (const auto& expr : funcAccess.actualArguments)
				{
					args.push_back(pEnv->expand(boost::apply_visitor(*this, expr)));
				}
				Expr caller = FunctionCaller(function, args);
				const Evaluated returnedValue = pEnv->expand(boost::apply_visitor(*this, caller));
				address = pEnv->makeTemporaryValue(returnedValue);
				*/

				std::vector<Address> args;
				for (const auto& expr : funcAccess.actualArguments)
				{
					args.push_back(pEnv->makeTemporaryValue(boost::apply_visitor(*this, expr)));
				}

				const Evaluated returnedValue = callFunction(function, args);
				address = pEnv->makeTemporaryValue(returnedValue);
			}
		}

		if (auto opt = expandFreeOpt(address))
		{
			return opt.value();
		}
		return pEnv->expand(address);
	}
}
