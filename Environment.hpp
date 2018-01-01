#pragma once
#include <stack>
#include "Node.hpp"

namespace cgl
{
	class Values
	{
	public:

		using ValueList = std::unordered_map<Address, Evaluated>;

		Address add(const Evaluated& value)
		{
			m_values.insert({ newAddress(), value });

			return Address(m_ID);
		}

		size_t size()const
		{
			return m_values.size();
		}

		Evaluated& operator[](Address key)
		{
			auto it = m_values.find(key);
			if (it == m_values.end())
			{
				std::cerr << "Error(" << __LINE__ << ")\n";
			}
			return it->second;
		}

		const Evaluated& operator[](Address key)const
		{
			auto it = m_values.find(key);
			if (it == m_values.end())
			{
				std::cerr << "Error(" << __LINE__ << ")\n";
			}
			return it->second;
		}

		/*boost::optional<const Evaluated&> at(Address key)const
		{
			auto it = m_values.find(key);
			if (it == m_values.end())
			{
				return boost::none;
			}
			return it->second;
		}*/

		ValueList::iterator at(Address key)
		{
			return m_values.find(key);
		}

		ValueList::const_iterator at(Address key)const
		{
			return m_values.find(key);
		}

		ValueList::const_iterator begin()const
		{
			return m_values.cbegin();
		}

		ValueList::const_iterator end()const
		{
			return m_values.cend();
		}

	private:

		/*unsigned nextID()
		{
			return ++m_ID;
		}*/

		Address newAddress()
		{
			return Address(++m_ID);
		}

		ValueList m_values;

		unsigned m_ID = 0;
	};

	class Environment
	{
	public:

		//using Scope = std::unordered_map<std::string, unsigned>;
		using Scope = std::unordered_map<std::string, Address>;

		using LocalEnvironment = std::vector<Scope>;

		//ObjectReference makeFuncVal(const std::vector<Identifier>& arguments, const Expr& expr);
		Address makeFuncVal(std::shared_ptr<Environment> pEnv, const std::vector<Identifier>& arguments, const Expr& expr);

		//�X�R�[�v�̓����ɓ���/�o��
		void enterScope()
		{
			//m_variables.emplace_back();
			localEnv().emplace_back();
		}
		void exitScope()
		{
			//m_variables.pop_back();
			localEnv().pop_back();
		}

		//�֐��Ăяo���ȂǕʂ̃X�R�[�v�ɐ؂�ւ���/�߂�
		/*
		void switchFrontScope(int switchDepth)
		{
			//�֐��̃X�R�[�v���������̓���͖��m�F

			std::cout << "FuncScope:" << switchDepth << std::endl;
			std::cout << "Variables:" << m_variables.size() << std::endl;

			m_diffScopes.push({ switchDepth,std::vector<Scope>(m_variables.begin() + switchDepth + 1, m_variables.end()) });
			m_variables.erase(m_variables.begin() + switchDepth + 1, m_variables.end());
		}
		void switchBackScope()
		{
			const int switchDepth = m_diffScopes.top().first;
			const auto& diffScope = m_diffScopes.top().second;

			m_variables.erase(m_variables.begin() + switchDepth + 1, m_variables.end());
			m_variables.insert(m_variables.end(), diffScope.begin(), diffScope.end());
			m_diffScopes.pop();
		}
		*/
		void switchFrontScope()
		{
			//�֐��̃X�R�[�v���������̓���͖��m�F
			m_localEnvStack.push(LocalEnvironment());
		}
		void switchBackScope()
		{
			m_localEnvStack.pop();
		}

		/*
		boost::optional<Address> find(const std::string& name)const
		{
			const auto valueIDOpt = findAddress(name);
			if (!valueIDOpt)
			{
				std::cerr << "Error(" << __LINE__ << ")\n";
				return boost::none;
			}

			//return m_values[valueIDOpt.value()];
		}
		*/

		/*bool isValid(Address address)const
		{
			return m_values.at(address) != m_values.end();
		}*/

		//Address dereference(const Evaluated& reference);
		/*
		boost::optional<const Evaluated&> dereference(const Evaluated& reference)const
		{
			if (!IsType<Address>(reference))
			{
				return boost::none;
			}

			//return m_values.at(As<Address>(reference));

			auto it = m_values.at(As<Address>(reference));
			if (it == m_values.end())
			{
				return boost::none;
			}

			return it->second;
		}
		*/

		/*Evaluated expandRef(const Evaluated& reference)const
		{
			if (!IsType<Address>(reference))
			{
				return reference;
			}

			if (Address address = As<Address>(reference))
			{
				if (auto opt = m_values.at(address))
				{
					return expandRef(opt.value());
				}
				else
				{
					CGL_Error("reference error");
					return 0;
				}
			}

			CGL_Error("reference error");
			return 0;
		}*/

		const Evaluated& expand(const LRValue& lrvalue)const
		{
			if (lrvalue.isLValue())
			{
				auto it = m_values.at(lrvalue.address());
				if (it != m_values.end())
				{
					return it->second;
				}

				CGL_Error("reference error");
			}

			return lrvalue.evaluated();
		}

		boost::optional<const Evaluated&> expandOpt(const LRValue& lrvalue)const
		{
			if (lrvalue.isLValue())
			{
				auto it = m_values.at(lrvalue.address());
				if (it != m_values.end())
				{
					return it->second;
				}

				return boost::none;
			}

			return lrvalue.evaluated();
		}

		Address evalReference(const Accessor& access);

		Expr expandFunction(const Expr& expr);

		//reference�Ŏw�����I�u�W�F�N�g�̒��ɂ���S�Ă̒l�ւ̎Q�Ƃ����X�g�Ŏ擾����
		/*std::vector<ObjectReference> expandReferences(const ObjectReference& reference, std::vector<ObjectReference>& output);
		std::vector<ObjectReference> expandReferences(const ObjectReference& reference)*/
		std::vector<Address> expandReferences(Address reference);

		//{a=1,b=[2,3]}, [a, b] => [1, [2, 3]]
		/*
		Evaluated expandList(const Evaluated& reference)
		{
			if (auto listOpt = AsOpt<List>(reference))
			{
				List expanded;
				for (const auto& elem : listOpt.value().data)
				{
					expanded.append(expandList(elem));
				}
				return expanded;
			}

			return dereference(reference);
		}
		*/
		
		//���[�J���ϐ���S�ēW�J����
		//�֐��̖߂�l�ȂǃX�R�[�v���ς�鎞�ɂ͎Q�Ƃ������p���Ȃ��̂ň�x�S�ēW�J����K�v������
		/*
		Evaluated expandObject(const Evaluated& reference)
		{
			if (auto opt = AsOpt<Record>(reference))
			{
				Record expanded;
				for (const auto& elem : opt.value().values)
				{
					expanded.append(elem.first, expandObject(elem.second));
				}
				return expanded;
			}
			else if (auto opt = AsOpt<List>(reference))
			{
				List expanded;
				for (const auto& elem : opt.value().data)
				{
					expanded.append(expandObject(elem));
				}
				return expanded;
			}

			return dereference(reference);
		}
		*/

		/*void bindObjectRef(const std::string& name, const ObjectReference& ref)
		{
			if (auto valueIDOpt = AsOpt<unsigned>(ref.headValue))
			{
				bindValueID(name, valueIDOpt.value());
			}
			else
			{
				const auto& valueRhs = dereference(ref);
				bindNewValue(name, valueRhs);
			}
		}*/
		void bindObjectRef(const std::string& name, Address ref)
		{
			bindValueID(name, ref);
		}

		void bindNewValue(const std::string& name, const Evaluated& value)
		{
			CGL_DebugLog("");
			//const Address newAddress = m_values.add(expandRef(value));
			const Address newAddress = m_values.add(value);
			bindValueID(name, newAddress);
		}

		void bindReference(const std::string& nameLhs, const std::string& nameRhs)
		{
			const Address address = findAddress(nameRhs);
			if (!address.isValid())
			{
				std::cerr << "Error(" << __LINE__ << ")\n";
				return;
			}

			bindValueID(nameLhs, address);
		}

		/*
		void bindValueID(const std::string& name, unsigned valueID)
		{
			//���R�[�h
			//���R�[�h����:���@�����K�w�ɓ�����:��������ꍇ�͂���ւ̍đ���A�����ꍇ�͐V���ɒ�`
			//���R�[�h����=���@�����K�w�ɓ�����:��������ꍇ�͂���ւ̍đ���A�����ꍇ�͂��̃X�R�[�v���ł̂ݗL���Ȓl�̃G�C���A�X�i�X�R�[�v�𔲂����猳�ɖ߂���Օ��j

			//���݂̊��ɕϐ������݂��Ȃ���΁A
			//�����X�g�̖����i���ł������̃X�R�[�v�j�ɕϐ���ǉ�����
			m_bindingNames.back().bind(name, valueID);
		}
		*/

		//void bindValueID(const std::string& name, unsigned ID)
		/*void bindValueID(const std::string& name, const Address ID)
		{
			for (auto scopeIt = m_variables.rbegin(); scopeIt != m_variables.rend(); ++scopeIt)
			{
				auto valIt = scopeIt->find(name);
				if (valIt != scopeIt->end())
				{
					valIt->second = ID;
					return;
				}
			}

			m_variables.back()[name] = ID;
		}*/
		void bindValueID(const std::string& name, const Address ID)
		{
			CGL_DebugLog("");
			for (auto scopeIt = localEnv().rbegin(); scopeIt != localEnv().rend(); ++scopeIt)
			{
				auto valIt = scopeIt->find(name);
				if (valIt != scopeIt->end())
				{
					valIt->second = ID;
					return;
				}
			}
			CGL_DebugLog("");

			localEnv().back()[name] = ID;
		}

		/*void push()
		{
			m_bindingNames.emplace_back(LocalEnvironment::Type::NormalScope);
		}

		void pop()
		{
			m_bindingNames.pop_back();
		}*/

		void printEnvironment()const;

		//void assignToObject(const ObjectReference& objectRef, const Evaluated& newValue);
		void assignToObject(Address address, const Evaluated& newValue)
		{
			m_values[address] = newValue;
			
			//m_values[address] = expandRef(newValue);
		}

		//����Ő������H
		void assignAddress(Address addressTo, Address addressFrom)
		{
			m_values[addressTo] = m_values[addressFrom];
			//m_values[addressTo] = expandRef(m_values[addressFrom]);
		}

		static std::shared_ptr<Environment> Make()
		{
			auto p = std::make_shared<Environment>();
			p->m_weakThis = p;
			p->switchFrontScope();
			return p;
		}

		static std::shared_ptr<Environment> Make(const Environment& other)
		{
			auto p = std::make_shared<Environment>(other);
			p->m_weakThis = p;
			return p;
		}

		//�l������ĕԂ��i�ϐ��ő�������Ȃ����̂�GC���������瑦���ɏ������j
		//���̕]���r����GC�͑���Ȃ��悤�ɂ���ׂ����H
		Address makeTemporaryValue(const Evaluated& value)
		{
			const Address address = m_values.add(value);

			//�֐��̓X�R�[�v�𔲂��鎞�ɒ�`�����̕ϐ����������Ȃ����Ď�����K�v������̂�ID��ۑ����Ă���
			/*if (IsType<FuncVal>(value))
			{
				m_funcValIDs.push_back(address);
			}*/

			return address;
		}

		Environment() = default;

/*
�����Ɍ��꓾�鎯�ʎq�͎���3��ނɕ�������B

1. �R�����̍����ɏo�Ă��鎯�ʎq�F
�@�@�ł������̃X�R�[�v�ɂ��̕ϐ����L��΂��̕ϐ��ւ̎Q��
�@�@�@�@�@�@�@�@�@�@�@�@�@�@�@�@�@������ΐV����������ϐ��ւ̑���

2. �C�R�[���̍����ɏo�Ă��鎯�ʎq�F
�@�@�X�R�[�v�̂ǂ����ɂ��̕ϐ����L��΂��̕ϐ��ւ̎Q��
�@�@�@�@�@�@�@�@�@�@�@�@�@�@�@�@������ΐV����������ϐ��ւ̑���

3. ����ȊO�̏ꏊ�ɏo�Ă��鎯�ʎq�F
�@�@�X�R�[�v�̂ǂ����ɂ��̕ϐ����L��΂��̕ϐ��ւ̎Q��
�@�@�@�@�@�@�@�@�@�@�@�@�@�@�@�@������Ζ����ȎQ�Ɓi�G���[�j

�����ŁA1�̗p�@��2,3�̗p�@�𗼗����邱�Ƃ͓���i���ʎq�����������ł͉���Ԃ���������ł��Ȃ��̂Łj�B
�������A1�̗p�@�͂��Ȃ����ł��邽�߁A�P�ɓ��ʈ������Ă��悢�C������B
�܂�A�R�����̍����ɏo�Ă����̂͒P��̎��ʎq�݂̂Ƃ���i���G�Ȃ��̂������Ă�����قǃ����b�g���Ȃ��f�o�b�O����ςɂȂ邾���j�B
����ɂ��A�R���������������ɒ��̎��ʎq���ꏏ�Ɍ���΍ςނ̂ŁA��L�̗p�@�𗼗��ł���B
*/
		/*boost::optional<Address> findValueID(const std::string& name)const
		{
			for (auto scopeIt = m_variables.rbegin(); scopeIt != m_variables.rend(); ++scopeIt)
			{
				auto variableIt = scopeIt->find(name);
				if (variableIt != scopeIt->end())
				{
					return variableIt->second;
				}
			}

			return boost::none;
		}*/

		
		/*Address findAddress(const std::string& name)const
		{
			for (auto scopeIt = m_variables.rbegin(); scopeIt != m_variables.rend(); ++scopeIt)
			{
				auto variableIt = scopeIt->find(name);
				if (variableIt != scopeIt->end())
				{
					return variableIt->second;
				}
			}

			return Address::Null();
		}*/
		Address findAddress(const std::string& name)const
		{
			for (auto scopeIt = localEnv().rbegin(); scopeIt != localEnv().rend(); ++scopeIt)
			{
				auto variableIt = scopeIt->find(name);
				if (variableIt != scopeIt->end())
				{
					return variableIt->second;
				}
			}

			return Address::Null();
		}

	private:

		LocalEnvironment& localEnv()
		{
			return m_localEnvStack.top();
		}

		const LocalEnvironment& localEnv()const
		{
			return m_localEnvStack.top();
		}

		/*int scopeDepth()const
		{
			return static_cast<int>(m_variables.size()) - 1;
		}*/

		//���ݎQ�Ɖ\�ȕϐ����̃��X�g�̃��X�g��Ԃ�
		/*
		std::vector<std::set<std::string>> currentReferenceableVariables()const
		{
			std::vector<std::set<std::string>> result;
			for (const auto& scope : m_variables)
			{
				result.emplace_back();
				auto& currentScope = result.back();
				for (const auto& var : scope)
				{
					currentScope.insert(var.first);
				}
			}

			return result;
		}
		*/

		//�����̃X�R�[�v���珇�Ԃɕϐ���T���ĕԂ�
		/*boost::optional<unsigned> findValueID(const std::string& name)const
		{
			boost::optional<unsigned> valueIDOpt;

			for (auto it = m_bindingNames.rbegin(); it != m_bindingNames.rend(); ++it)
			{
				valueIDOpt = it->find(name);
				if (valueIDOpt)
				{
					break;
				}
			}

			return valueIDOpt;
		}*/

		void garbageCollect();

		Values m_values;

		//�ϐ��̓X�R�[�v�P�ʂŊǗ������
		//�X�R�[�v�𔲂����炻�̃X�R�[�v�ŊǗ����Ă���ϐ��������ƍ폜����
		//���������Ċ��̓l�X�g�̐󂢏��Ƀ��X�g�ŊǗ����邱�Ƃ��ł���i�����[���̊�������݂��邱�Ƃ͂Ȃ��j
		//���X�g�̍ŏ��̗v�f�̓O���[�o���ϐ��Ƃ���Ƃ���
		//std::vector<LocalEnvironment> m_bindingNames;

		/*std::vector<Scope> m_variables;
		std::stack<std::pair<int, std::vector<Scope>>> m_diffScopes;*/
		
		
		std::stack<LocalEnvironment> m_localEnvStack;

		//std::vector<Address> m_funcValIDs;

		std::weak_ptr<Environment> m_weakThis;
	};
}