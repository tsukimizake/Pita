#pragma once
#include <iomanip>
#include "Node.hpp"
#include "Context.hpp"

namespace cgl
{
	template<typename T>
	inline std::string ToS(T str)
	{
		return std::to_string(str);
	}

	template<typename T>
	inline std::string ToS(T str, int precision)
	{
		std::ostringstream os;
		os << std::setprecision(precision) << str;
		return os.str();
	}

	class ValuePrinter : public boost::static_visitor<void>
	{
	public:
		ValuePrinter(std::shared_ptr<Context> pEnv, std::ostream& os, int indent, const std::string& header = "") :
			pEnv(pEnv),
			os(os),
			m_indent(indent),
			m_header(header)
		{}

		std::shared_ptr<Context> pEnv;
		int m_indent;
		std::ostream& os;
		mutable std::string m_header;
		
		std::string indent()const;

		void operator()(bool node)const;

		void operator()(int node)const;

		void operator()(double node)const;

		void operator()(const CharString& node)const;

		void operator()(const List& node)const;

		void operator()(const KeyValue& node)const;

		void operator()(const Record& node)const;

		void operator()(const FuncVal& node)const;

		void operator()(const Jump& node)const;
	};

	inline void printVal(const Val& evaluated, std::shared_ptr<Context> pEnv, std::ostream& os, int indent = 0)
	{
		ValuePrinter printer(pEnv, os, indent);
		boost::apply_visitor(printer, evaluated);
	}

#ifdef CGL_EnableLogOutput
	inline void printVal(const Val& evaluated, std::shared_ptr<Context> pEnv, int indent = 0)
	{
		printVal(evaluated, pEnv, ofs, indent);
	}
#else
	inline void printVal(const Val& evaluated, std::shared_ptr<Context> pEnv, int indent = 0) {}
#endif

	class PrintSatExpr : public boost::static_visitor<void>
	{
	public:
		PrintSatExpr(const std::vector<double>& data, std::ostream& os) :
			data(data),
			os(os)
		{}

		const std::vector<double>& data;
		std::ostream& os;

		void operator()(double node)const;

		void operator()(const SatReference& node)const;

		void operator()(const SatUnaryExpr& node)const;

		void operator()(const SatBinaryExpr& node)const;

		void operator()(const SatFunctionReference& node)const;
	};

	class Printer : public boost::static_visitor<void>
	{
	public:
		Printer(std::shared_ptr<Context> pEnv, std::ostream& os, int indent = 0) :
			pEnv(pEnv),
			os(os),
			m_indent(indent)
		{}

		std::shared_ptr<Context> pEnv;
		std::ostream& os;
		int m_indent;

		std::string indent()const;

		void operator()(const LRValue& node)const;

		void operator()(const Identifier& node)const;

		void operator()(const SatReference& node)const;

		void operator()(const UnaryExpr& node)const;

		void operator()(const BinaryExpr& node)const;

		void operator()(const DefFunc& defFunc)const;

		void operator()(const Range& range)const;

		void operator()(const Lines& statement)const;

		void operator()(const If& if_statement)const;
		
		void operator()(const For& forExpression)const;

		void operator()(const Return& return_statement)const;

		void operator()(const ListConstractor& listConstractor)const;

		void operator()(const KeyExpr& keyExpr)const;

		void operator()(const RecordConstractor& recordConstractor)const;

		void operator()(const RecordInheritor& record)const;

		void operator()(const DeclSat& node)const;

		void operator()(const DeclFree& node)const;

		void operator()(const Accessor& accessor)const;
	};

	inline void printExpr(const Expr& expr, std::shared_ptr<Context> pEnv, std::ostream& os)
	{
		os << "PrintExpr(\n";
		boost::apply_visitor(Printer(pEnv, os), expr);
		os << ") " << std::endl;
	}

#ifdef CGL_EnableLogOutput
	inline void printExpr(const Expr& expr)
	{
		printExpr(expr, ofs);
	}
#else
	inline void printExpr(const Expr& expr) {}
#endif

}
