#include <iostream>
#include <math.h>
#include <vector>
#include <fstream>
#include <string>
#include <stack>
#include <map>

class Equation {
public:
	int precedence(char c) {
		if (c == '+' || c == '-')
			return 1;
		else if (c == '*' || c == '/')
			return 2;
		else if (c == '^')
			return 3;
		else
			return -1;
	}

	bool isFunction(std::string term) {
		return (term == "sin" || term == "cos" || term == "tan" || term == "log");
	}

	bool isUnaryMinus(char c, size_t pos, const std::string& infix) {
		return c == '-' && (pos == 0 || infix[pos - 1] == '(' || isOperator(infix[pos - 1]) || infix[pos - 1] == ' ');
	}

	bool isOperator(char c) {
		return c == '+' || c == '-' || c == '*' || c == '/' || c == '^';
	}

	std::vector<std::string> infixToPostfix(std::string infix) {
		std::stack<std::string> st;
		std::vector<std::string> postfix;
		std::string token;
		bool lastWasOperator = true;

		for (int i = 0; i < infix.size(); i++) {
			char c = infix[i];
			if (std::isdigit(c) || c == '.' || std::isalpha(c)) {
				token += c;
				lastWasOperator = false;
			}
			else {
				if (!token.empty()) {
					postfix.push_back(token);
					token.clear();
				}
				if (c == '(') {
					st.push(std::string(1, c));
				}
				else if (c == ')') {
					while (!st.empty() && st.top() != "(") {
						postfix.push_back(st.top());
						st.pop();
					}
					st.pop();
				}
				else if (isOperator(c)) {
					if (isUnaryMinus(c, i, infix)) {
						token += c;
					}
					else {
						while (!st.empty() && st.top() != "(" && precedence(c) <= precedence(st.top()[0])) {
							postfix.push_back(st.top());
							st.pop();
						}
						st.push(std::string(1, c));
						lastWasOperator = true;
					}
				}
			}
		}
		if (!token.empty()) {
			postfix.push_back(token);
		}
		while (!st.empty()) {
			postfix.push_back(st.top());
			st.pop();
		}

		return postfix;
	}

	double calculatePostfix(std::vector<std::string> postfix) {
		std::stack<double> opstack;

		for (const std::string& token : postfix) {
			if (isOperator(token[0]) && token.size() == 1) {
				double first, second, result;
				if (opstack.size() < 1) {
					std::cout << "Not enough operands for operator: " << token << std::endl;
					continue;
				}
				else {
					second = opstack.top();
					opstack.pop();
					first = opstack.top();
					opstack.pop();
				}
				switch (token[0]) {
				case '+':
					result = first + second;
					break;
				case '-':
					result = first - second;
					break;
				case '*':
					result = first * second;
					break;
				case '/':
					if (second == 0) {
						std::cout << "Division by zero" << std::endl;
					}
					result = first / second;
					break;
				case '^':
					result = std::pow(first, second);
					break;
				default:
					std::cout << "Unknown operator: " << token << std::endl;
				}
				opstack.push(result);
			}
			else if (isFunction(token)) {
				if (opstack.empty()) {
					std::cout << "Not enough operators for function: " << token << std::endl;
				}

				double operand = opstack.top();
				opstack.pop();

				double result;
				if (token == "sin")
					result = std::sin(operand);
				else if (token == "cos")
					result = std::cos(operand);
				else if (token == "tan")
					result = std::tan(operand);
				else if (token == "log")
					result = std::log10(operand);
				else
					std::cout << "Unknown function: " << token << std::endl;
				opstack.push(result);
			}
			else {
				try {
					double number = std::stod(token);
					opstack.push(number);
				}
				catch (const std::exception& e) {
					std::cout << "Invalid token: " << token << std::endl;
				}
			}
		}
		if (opstack.size() != 1) {
			std::cout << "Invalid postfix expression" << std::endl;
		}
		return opstack.top();
	}

	double evaluate(std::string expression) {
		std::vector<std::string> postfix = infixToPostfix(expression);
		return calculatePostfix(postfix);
	}

	double evaluateWithXY(std::string expression, double x, double y) {
		size_t xPos = expression.find("x");
		while (xPos != std::string::npos) {
			expression.replace(xPos, 1, "(" + std::to_string(x) + ")");
			xPos = expression.find("x", xPos + 1);
		}

		size_t yPos = expression.find("y");
		while (yPos != std::string::npos) {
			expression.replace(yPos, 1, "(" + std::to_string(y) + ")");
			yPos = expression.find("y", yPos + 1);
		}

		return evaluate(expression);
	}
};