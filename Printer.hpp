#pragma once
#include "Node.hpp"

namespace cgl
{
	class ValuePrinter : public boost::static_visitor<void>
	{
	public:
		ValuePrinter(int indent = 0, const std::string& header = "") :
			m_indent(indent),
			m_header(header)
		{}

		int m_indent;
		mutable std::string m_header;

		std::string indent()const
		{
			const int tabSize = 4;
			std::string str;
			for (int i = 0; i < m_indent*tabSize; ++i)
			{
				str += ' ';
			}
			str += m_header;
			m_header.clear();
			return str;
		}

		void operator()(bool node)const
		{
			std::cout << indent() << "Bool(" << (node ? "true" : "false") << ")" << std::endl;
		}

		void operator()(int node)const
		{
			std::cout << indent() << "Int(" << node << ")" << std::endl;
		}

		void operator()(double node)const
		{
			std::cout << indent() << "Double(" << node << ")" << std::endl;
		}

		/*void operator()(Address node)const
		{
			std::cout << indent() << "Address(" << node.toString() << ")" << std::endl;
		}*/

		void operator()(const List& node)const
		{
			std::cout << indent() << "[" << std::endl;
			const auto& data = node.data;

			for (size_t i = 0; i < data.size(); ++i)
			{
				/*const std::string header = std::string("(") + std::to_string(i) + "): ";
				const auto child = ValuePrinter(m_indent + 1, header);
				const Evaluated currentData = data[i];
				boost::apply_visitor(child, currentData);*/

				const std::string header = std::string("(") + std::to_string(i) + "): ";
				const auto child = ValuePrinter(m_indent + 1, header);
				std::cout << child.indent() << "Address(" << data[i].toString() << ")" << std::endl;
			}

			std::cout << indent() << "]" << std::endl;
		}

		void operator()(const KeyValue& node)const
		{
			const std::string header = static_cast<std::string>(node.name) + ": ";
			{
				const auto child = ValuePrinter(m_indent, header);
				boost::apply_visitor(child, node.value);
			}
		}

		void operator()(const Record& node)const
		{
			std::cout << indent() << "{" << std::endl;

			for (const auto& value : node.values)
			{
				const std::string header = value.first + ": ";

				const auto child = ValuePrinter(m_indent + 1, header);
				//const Evaluated evaluated = value.second;
				//boost::apply_visitor(child, evaluated);

				std::cout << child.indent() << "Address(" << value.second.toString() << ")" << std::endl;
			}

			std::cout << indent() << "}" << std::endl;
		}

		void operator()(const FuncVal& node)const;

		void operator()(const Jump& node)const
		{
			std::cout << indent() << "Jump(" << node.op << ")" << std::endl;
		}

		void operator()(const DeclSat& node)const
		{
			std::cout << indent() << "DeclSat(" << ")" << std::endl;
		}

		void operator()(const DeclFree& node)const
		{
			std::cout << indent() << "DeclFree(" << ")" << std::endl;
		}
	};

	inline void printEvaluated(const Evaluated& evaluated, int indent = 0)
	{
		ValuePrinter printer(indent);
		boost::apply_visitor(printer, evaluated);
	}

	class PrintSatExpr : public boost::static_visitor<void>
	{
	public:
		const std::vector<double>& data;

		PrintSatExpr(const std::vector<double>& data) :
			data(data)
		{}

		void operator()(double node)const
		{
			std::cout << node;
		}

		void operator()(const SatReference& node)const
		{
			std::cout << "$" << node.refID;
		}

		void operator()(const SatUnaryExpr& node)const
		{
			switch (node.op)
			{
				//case UnaryOp::Not:   return lhs;
			case UnaryOp::Minus: std::cout << "-( "; break;
			case UnaryOp::Plus:  std::cout << "+( "; break;
			}

			boost::apply_visitor(*this, node.lhs);
			std::cout << " )";
		}

		void operator()(const SatBinaryExpr& node)const
		{
			std::cout << "( ";
			boost::apply_visitor(*this, node.lhs);

			switch (node.op)
			{
			case BinaryOp::And: std::cout << " & "; break;

			case BinaryOp::Or:  std::cout << " | "; break;

			case BinaryOp::Equal:        std::cout << " == "; break;
			case BinaryOp::NotEqual:     std::cout << " != "; break;
			case BinaryOp::LessThan:     std::cout << " < "; break;
			case BinaryOp::LessEqual:    std::cout << " <= "; break;
			case BinaryOp::GreaterThan:  std::cout << " > "; break;
			case BinaryOp::GreaterEqual: std::cout << " >= "; break;

			case BinaryOp::Add: std::cout << " + "; break;
			case BinaryOp::Sub: std::cout << " - "; break;
			case BinaryOp::Mul: std::cout << " * "; break;
			case BinaryOp::Div: std::cout << " / "; break;

			case BinaryOp::Pow: std::cout << " ^ "; break;
			}

			boost::apply_visitor(*this, node.rhs);
			std::cout << " )";
		}

		void operator()(const SatFunctionReference& node)const
		{
		}
	};

	class Printer : public boost::static_visitor<void>
	{
	public:
		Printer(int indent = 0)
			:m_indent(indent)
		{}

		int m_indent;

		std::string indent()const
		{
			const int tabSize = 4;
			std::string str;
			for (int i = 0; i < m_indent*tabSize; ++i)
			{
				str += ' ';
			}
			return str;
		}

		void operator()(const LRValue& node)const
		{
			if (node.isLValue())
			{
				//std::cout << indent() << "Address(" << node.address() << ")" << std::endl;
				std::cout << indent() << "Address(" << node.address().toString() << ")" << std::endl;
			}
			else
			{
				printEvaluated(node.evaluated(), m_indent);
			}
			//printEvaluated(node.value, m_indent);
			//std::cout << indent() << "Double(" << node << ")" << std::endl;
		}

		void operator()(const Identifier& node)const
		{
			std::cout << indent() << "Identifier(" << static_cast<std::string>(node) << ")" << std::endl;
		}

		void operator()(const UnaryExpr& node)const
		{
			std::cout << indent() << "Unary(" << std::endl;

			boost::apply_visitor(Printer(m_indent + 1), node.lhs);

			std::cout << indent() << ")" << std::endl;
		}

		void operator()(const BinaryExpr& node)const
		{
			std::cout << indent() << "Binary(" << std::endl;
			boost::apply_visitor(Printer(m_indent + 1), node.lhs);
			boost::apply_visitor(Printer(m_indent + 1), node.rhs);
			std::cout << indent() << ")" << std::endl;
		}

		void operator()(const DefFunc& defFunc)const
		{
			std::cout << indent() << "DefFunc(" << std::endl;

			{
				const auto child = Printer(m_indent + 1);
				std::cout << child.indent() << "Arguments(" << std::endl;
				{
					const auto grandChild = Printer(child.m_indent + 1);

					const auto& args = defFunc.arguments;
					for (size_t i = 0; i < args.size(); ++i)
					{
						std::cout << grandChild.indent() << static_cast<std::string>(args[i]) << (i + 1 == args.size() ? "\n" : ",\n");
					}
				}
				std::cout << child.indent() << ")" << std::endl;
			}

			boost::apply_visitor(Printer(m_indent + 1), defFunc.expr);

			std::cout << indent() << ")" << std::endl;
		}

		void operator()(const FunctionCaller& callFunc)const
		{
			std::cout << indent() << "FunctionCaller(" << std::endl;

			{
				const auto child = Printer(m_indent + 1);
				std::cout << child.indent() << "FuncRef(" << std::endl;

				if (IsType<Identifier>(callFunc.funcRef))
				{
					const auto& funcName = As<Identifier>(callFunc.funcRef);
					Expr expr(funcName);
					boost::apply_visitor(Printer(m_indent + 2), expr);
				}
				else
				{
					std::cout << indent() << "(FuncVal)" << std::endl;
				}

				std::cout << child.indent() << ")" << std::endl;
			}

			{
				/*const auto child = Printer(m_indent + 1);
				std::cout << child.indent() << "Arguments(" << std::endl;

				const auto& args = callFunc.actualArguments;
				for (size_t i = 0; i < args.size(); ++i)
				{
				boost::apply_visitor(Printer(m_indent + 2), args[i]);
				}

				std::cout << child.indent() << ")" << std::endl;
				*/
			}

			std::cout << indent() << ")" << std::endl;
		}

		void operator()(const Range& range)const
		{
			std::cout << indent() << "Range(" << std::endl;
			boost::apply_visitor(Printer(m_indent + 1), range.lhs);
			boost::apply_visitor(Printer(m_indent + 1), range.rhs);
			std::cout << indent() << ")" << std::endl;
		}

		void operator()(const Lines& statement)const
		{
			std::cout << indent() << "Statement begin" << std::endl;

			int i = 0;
			for (const auto& expr : statement.exprs)
			{
				std::cout << indent() << "(" << i << "): " << std::endl;
				boost::apply_visitor(Printer(m_indent + 1), expr);
				++i;
			}

			std::cout << indent() << "Statement end" << std::endl;
		}

		void operator()(const If& if_statement)const
		{
			std::cout << indent() << "If(" << std::endl;

			{
				const auto child = Printer(m_indent + 1);
				std::cout << child.indent() << "Cond(" << std::endl;

				boost::apply_visitor(Printer(m_indent + 2), if_statement.cond_expr);

				std::cout << child.indent() << ")" << std::endl;
			}

			{
				const auto child = Printer(m_indent + 1);
				std::cout << child.indent() << "Then(" << std::endl;

				boost::apply_visitor(Printer(m_indent + 2), if_statement.then_expr);

				std::cout << child.indent() << ")" << std::endl;
			}

			if (if_statement.else_expr)
			{
				const auto child = Printer(m_indent + 1);
				std::cout << child.indent() << "Else(" << std::endl;

				boost::apply_visitor(Printer(m_indent + 2), if_statement.else_expr.value());

				std::cout << child.indent() << ")" << std::endl;
			}

			std::cout << indent() << ")" << std::endl;
		}

		void operator()(const For& forExpression)const
		{
			std::cout << indent() << "For(" << std::endl;

			std::cout << indent() << ")" << std::endl;
		}

		void operator()(const Return& return_statement)const
		{
			std::cout << indent() << "Return(" << std::endl;

			boost::apply_visitor(Printer(m_indent + 1), return_statement.expr);

			std::cout << indent() << ")" << std::endl;
		}

		void operator()(const ListConstractor& listConstractor)const
		{
			std::cout << indent() << "ListConstractor(" << std::endl;

			int i = 0;
			for (const auto& expr : listConstractor.data)
			{
				std::cout << indent() << "(" << i << "): " << std::endl;
				boost::apply_visitor(Printer(m_indent + 1), expr);
				++i;
			}
			std::cout << indent() << ")" << std::endl;
		}

		void operator()(const KeyExpr& keyExpr)const
		{
			std::cout << indent() << "KeyExpr(" << std::endl;

			const auto child = Printer(m_indent + 1);

			std::cout << child.indent() << static_cast<std::string>(keyExpr.name) << std::endl;
			boost::apply_visitor(Printer(m_indent + 1), keyExpr.expr);
			std::cout << indent() << ")" << std::endl;
		}

		void operator()(const RecordConstractor& recordConstractor)const
		{
			std::cout << indent() << "RecordConstractor(" << std::endl;

			int i = 0;
			for (const auto& expr : recordConstractor.exprs)
			{
				std::cout << indent() << "(" << i << "): " << std::endl;
				boost::apply_visitor(Printer(m_indent + 1), expr);
				++i;
			}
			std::cout << indent() << ")" << std::endl;
		}

		void operator()(const RecordInheritor& record)const
		{
			;
		}

		void operator()(const Accessor& accessor)const
		{
			std::cout << indent() << "Accessor(" << std::endl;

			std::cout << indent() << ")" << std::endl;
		}
	};

	inline void printLines(const Lines& statement)
	{
		std::cout << "Lines begin" << std::endl;

		int i = 0;
		for (const auto& expr : statement.exprs)
		{
			std::cout << "Expr(" << i << "): " << std::endl;
			boost::apply_visitor(Printer(), expr);
			++i;
		}

		std::cout << "Lines end" << std::endl;
	}

	inline void printExpr(const Expr& expr)
	{
		std::cout << "PrintExpr(\n";
		boost::apply_visitor(Printer(), expr);
		std::cout << ") " << std::endl;
	}

	inline void ValuePrinter::operator()(const FuncVal& node)const
	{
		std::cout << indent() << "FuncVal(" << std::endl;

		{
			const auto child = ValuePrinter(m_indent + 1);
			std::cout << child.indent();
			for (size_t i = 0; i < node.arguments.size(); ++i)
			{
				std::cout << static_cast<std::string>(node.arguments[i]);
				if (i + 1 != node.arguments.size())
				{
					std::cout << ", ";
				}
			}
			std::cout << std::endl;

			std::cout << child.indent() << "->";

			boost::apply_visitor(Printer(m_indent + 1), node.expr);
		}

		std::cout << indent() << ")" << std::endl;
	}
}