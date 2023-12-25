#ifndef DEAMER_FP_H
#define DEAMER_FP_H

#include <iostream>
#include <memory>
#include <variant>
#include <map>
#include <set>
#include <vector>
#include <optional>
#include <cmath>

enum class FixpointOperation
{
    division,
    multiplication,
    addition,
    subtraction,
    next_layer_equivalence,
    parametrized_reference,
    parametrized_equivalence,

    // Special Function
    ceil,
    floor,
};

enum class FixpointParameterType
{
    integer,
};

enum class FixpointParameterGeneralType
{
    constant,
    variable,
};

template<typename T>
struct FixpointComputation;

template<typename T>
struct FixpointParameterComputation;

template<typename T>
struct FixpointCache;

struct FixpointParameter
{
public:
    std::variant<std::monostate, int> value;

    FixpointParameterType type;
    FixpointParameterGeneralType generalType;

    std::vector<FixpointParameter> children;
    std::optional<FixpointOperation> operation;

public:
    FixpointParameter(int value_)
        : value(value_), type(FixpointParameterType::integer), generalType(FixpointParameterGeneralType::constant)
    {}

    FixpointParameter()
        : type(FixpointParameterType::integer), generalType(FixpointParameterGeneralType::variable)
    {}

    template<typename T>
    FixpointParameter()
        : type(FixpointParameterType::integer), generalType(FixpointParameterGeneralType::variable)
    {}

public:
    FixpointParameter& operator=(int value_)
    {
        (*this).value = value_;
        type = FixpointParameterType::integer;
        generalType = FixpointParameterGeneralType::constant;
        return *this;
    }

    FixpointParameter operator-(int value_)
    {
        auto newFixpointParameter = FixpointParameter{};
        newFixpointParameter.children.push_back(*this);
        newFixpointParameter.children.push_back(value_);
        newFixpointParameter.operation = FixpointOperation::subtraction;
        return newFixpointParameter;
    }

    FixpointParameter operator+(int value_)
    {
        auto newFixpointParameter = FixpointParameter{};
        newFixpointParameter.children.push_back(*this);
        newFixpointParameter.children.push_back(value_);
        newFixpointParameter.operation = FixpointOperation::addition;
        return newFixpointParameter;
    }

    FixpointParameter operator*(int value_)
    {
        auto newFixpointParameter = FixpointParameter{};
        newFixpointParameter.children.push_back(*this);
        newFixpointParameter.children.push_back(value_);
        newFixpointParameter.operation = FixpointOperation::multiplication;
        return newFixpointParameter;
    }

    template<typename T>
    T evaluate(FixpointCache<T>& cache)
    {
        if (operation.has_value())
        {
            // Not final, requires modification
            switch (operation.value())
            {
            case FixpointOperation::addition: {
                return children[0].evaluate(cache) + children[1].evaluate(cache);
            }
            case FixpointOperation::subtraction: {
                return children[0].evaluate(cache) - children[1].evaluate(cache);
            }
            case FixpointOperation::multiplication: {
                return children[0].evaluate(cache) * children[1].evaluate(cache);
            }
            case FixpointOperation::division: {
                return children[0].evaluate(cache) / children[1].evaluate(cache);
            }
            case FixpointOperation::ceil: {
                return std::ceil(children[0].evaluate(cache));
            }
            case FixpointOperation::floor: {
                return std::floor(children[0].evaluate(cache));
            }
            case FixpointOperation::next_layer_equivalence:
            case FixpointOperation::parametrized_reference:
            case FixpointOperation::parametrized_equivalence: {
                break;
            }
            }

            throw std::logic_error("Unsupported or invalid operation.");
        }
        else
        {
            if (std::holds_alternative<std::monostate>(value))
            {
                return cache.get_parameter();
            }
            else if (std::holds_alternative<int>(value))
            {
                return std::get<int>(value);
            }
         
            throw std::logic_error("Unsupported or invalid type.");
        }
    }
};

template<typename T>
struct Fixpoint
{
public:
    T value;
    std::vector<std::unique_ptr<FixpointComputation<T>>> fixpointComputations;

public:
    Fixpoint(const T& rhs)
        : value(rhs)
    {
    }

    Fixpoint(const Fixpoint& rhs)
        : value(rhs.value)
    {
    }

    FixpointComputation<T> pattern_match(T t)
    {
        std::vector<FixpointComputation<T>> validMatches;
        for (auto& computation : fixpointComputations)
        {
            if (computation->match(t))
            {
                validMatches.push_back(*computation.get());
            }
        }

        return validMatches[0];
    }

    FixpointComputation<T> operator=(const FixpointComputation<T>& rhs);

    FixpointParameterComputation<T> operator()(FixpointParameter parameter);
};

template<typename T>
struct FixpointCache
{
public:
    std::map<Fixpoint<T>*, T> cacheFixpoints;

public:
    FixpointCache() = default;

public:
    bool contains(Fixpoint<T>* rhs) const
    {
        return cacheFixpoints.find(rhs) != cacheFixpoints.end();
    }

    T get(Fixpoint<T>* rhs) const
    {
        return cacheFixpoints.find(rhs)->second;
    }

    void remember(Fixpoint<T>* lhs, T rhs)
    {
        if (contains(lhs))
        {
            cacheFixpoints.find(lhs)->second = rhs;
        }
        else
        {
            cacheFixpoints.insert({lhs, rhs});
        }
    }

    std::optional<T> parameter;
    void register_parameter(T t)
    {
        parameter = t;
    }

    T get_parameter()
    {
        if (parameter.has_value())
        {
            return parameter.value();
        }
        else
        {
            throw std::logic_error("Invalid access.");
        }
    }
};

template<typename T>
struct FixpointReference
{
public:
    std::variant<Fixpoint<T>*, T> value;

public:
    FixpointReference(Fixpoint<T>& value_)
        : value(&value_)
    {
    }

    FixpointReference(Fixpoint<T>* value_)
        : value(value_)
    {
    }

    FixpointReference(const T& value_)
        : value(value_)
    {
    }

public:
    T ToT() const
    {
        if (std::holds_alternative<Fixpoint<T>*>(value))
        {
            return std::get<Fixpoint<T>*>(value)->value;
        }
        else if (std::holds_alternative<T>(value))
        {
            return std::get<T>(value);
        }

        throw std::logic_error("Unsupported or invalid type.");
    }

    T  ToT(FixpointCache<T>& cache) const
    {
        if (std::holds_alternative<Fixpoint<T>*>(value))
        {
            if (cache.contains(std::get<Fixpoint<T>*>(value)))
            {
                return cache.get(std::get<Fixpoint<T>*>(value));
            }

            cache.remember(std::get<Fixpoint<T>*>(value), std::get<Fixpoint<T>*>(value)->value);
            return cache.get(std::get<Fixpoint<T>*>(value));
        }
        else if (std::holds_alternative<T>(value))
        {
            return std::get<T>(value);
        }

        throw std::logic_error("Unsupported or invalid type.");
    }
};

template<typename T>
struct FixpointComputation
{
public:
    // std::vector<FixpointReference<T>> variables;
    // std::vector<FixpointOperation> operations;
    std::vector<std::variant<FixpointParameter, FixpointReference<T>, FixpointComputation<T>>> children;
    FixpointOperation operation;

public:
    FixpointComputation() = default;

public:
    T operator()()
    {
        FixpointCache<T> cache;
        return Computation(cache);
    }

    T operator()(T parameter1)
    {
        static int layer = 0;
        FixpointCache<T> cache;
        cache.register_parameter(parameter1);
        if (operation == FixpointOperation::parametrized_equivalence)
        {
            auto computationReference = std::get<FixpointComputation<T>>(children[0]);
            auto reference = std::get<FixpointReference<T>>(computationReference.children[0]);
            auto fixpointPtr = std::get<Fixpoint<T>*>(reference.value);
            auto computation = fixpointPtr->pattern_match(parameter1);

            // The value is child[1]
            auto value = computation.children[1];
            if (std::holds_alternative<FixpointComputation<T>>(value))
            {
                return std::get<FixpointComputation<T>>(value).Computation(cache);
            }
            else if (std::holds_alternative<FixpointReference<T>>(value))
            {
                return std::get<FixpointReference<T>>(value).ToT(cache);
            }

            throw std::logic_error("Unsupported or invalid computation.");
        }
        else
        {
            return Computation(cache);        
        }
    }

    T Computation(FixpointCache<T>& cache)
    {
        auto LocalParameterComputation = [&](std::size_t index) {
            if (std::holds_alternative<FixpointReference<T>>(children[index]))
            {
                return std::get<FixpointReference<T>>(children[index]).ToT(cache);
            }
            else if (std::holds_alternative<FixpointComputation<T>>(children[index]))
            {
                return std::get<FixpointComputation<T>>(children[index]).Computation(cache);
            }
            else if (std::holds_alternative<FixpointParameter>(children[index]))
            {
                return std::get<FixpointParameter>(children[index]).evaluate(cache);
            }

            throw std::logic_error("Unsupported or invalid type.");
        };

        switch (operation)
        {
        case FixpointOperation::division: {
            return LocalParameterComputation(0) / LocalParameterComputation(1);
        }
        case FixpointOperation::multiplication: {
            return LocalParameterComputation(0) * LocalParameterComputation(1);
        }
        case FixpointOperation::addition: {
            return LocalParameterComputation(0) + LocalParameterComputation(1);
        }
        case FixpointOperation::subtraction: {
            return LocalParameterComputation(0) - LocalParameterComputation(1);
        }
        case FixpointOperation::ceil: {
            return std::ceil(LocalParameterComputation(0));
        }
        case FixpointOperation::floor: {
            return std::floor(LocalParameterComputation(0));
        }
        case FixpointOperation::parametrized_reference: {
            auto fixpoint = std::get<Fixpoint<T>*>(std::get<FixpointReference<T>>(children[0]).value);
            auto evaluatedParameter = LocalParameterComputation(1);
            auto computation = fixpoint->pattern_match(evaluatedParameter);
            auto returnValue = computation(evaluatedParameter);
            return returnValue;
        }
        case FixpointOperation::parametrized_equivalence: {
            return -1;
        }
        case FixpointOperation::next_layer_equivalence: {
            T delta = 0.01;
            T newLayer{};
            T  oldLayer{};
            auto fixpoint = std::get<Fixpoint<T>*>(std::get<FixpointReference<T>>(children[0]).value);
            do
            {
                newLayer = LocalParameterComputation(1);
                oldLayer = cache.get(fixpoint);
                fixpoint->value = newLayer;
                cache.remember(fixpoint, newLayer);
                //std::cout << "Old: " << oldLayer << " New: " << newLayer << "\n";
            } while(std::abs(newLayer - oldLayer) > delta);
            return fixpoint->value;
        }
        }

        throw std::logic_error("Invalid operation.");
    }

    bool match(T t)
    {
        auto core = FixpointComputation<T>::children[0];
        auto coreComputation = std::get<FixpointComputation<T>>(core);
        auto parameter = std::get<FixpointParameter>(coreComputation.children[1]);
        if (parameter.generalType == FixpointParameterGeneralType::constant)
        {
            return std::holds_alternative<int>(parameter.value) && std::get<int>(parameter.value) == t;
        }
        else
        {
            return true;
        }
    }
};

template<typename T>
struct FixpointParameterComputation : public FixpointComputation<T>
{
public:

public:
    FixpointParameterComputation() = default;

public:
    FixpointComputation<T> operator=(const FixpointComputation<T>& rhs);

    FixpointComputation<T> operator=(const FixpointReference<T>& rhs);

    FixpointComputation<T> operator=(const T& rhs);

    FixpointComputation<T> operator=(Fixpoint<T>& rhs);
};

template<typename T>
FixpointComputation<T> operator/(const FixpointComputation<T>& lhs, const FixpointComputation<T>& rhs)
{
    auto newComputation = FixpointComputation<T>();
    newComputation.children.push_back(lhs);
    newComputation.children.push_back(rhs);
    newComputation.operation = FixpointOperation::division;
    return newComputation;
}

template<typename T>
FixpointComputation<T> operator/(const FixpointComputation<T>& lhs, const FixpointReference<T>& rhs)
{
    auto newComputation = FixpointComputation<T>();
    newComputation.children.push_back(lhs);
    newComputation.children.push_back(rhs);
    newComputation.operation = FixpointOperation::division;
    return newComputation;
}

template<typename T>
FixpointComputation<T> operator/(const FixpointReference<T>& lhs, const FixpointComputation<T>& rhs)
{
    auto newComputation = FixpointComputation<T>();
    newComputation.children.push_back(lhs);
    newComputation.children.push_back(rhs);
    newComputation.operation = FixpointOperation::division;
    return newComputation;
}

template<typename T>
FixpointComputation<T> operator/(const FixpointReference<T>& lhs, const FixpointReference<T>& rhs)
{
    auto newComputation = FixpointComputation<T>();
    newComputation.children.push_back(lhs);
    newComputation.children.push_back(rhs);
    newComputation.operation = FixpointOperation::division;
    return newComputation;
}

template<typename T>
FixpointComputation<T> operator/(const FixpointReference<T>& lhs, const Fixpoint<T>& rhs)
{
    auto newComputation = FixpointComputation<T>();
    newComputation.children.push_back(lhs);
    newComputation.children.push_back(rhs);
    newComputation.operation = FixpointOperation::division;
    return newComputation;
}

template<typename T>
FixpointComputation<T> operator/(Fixpoint<T>& lhs, const FixpointReference<T>& rhs)
{
    auto newComputation = FixpointComputation<T>();
    newComputation.children.push_back(lhs);
    newComputation.children.push_back(rhs);
    newComputation.operation = FixpointOperation::division;
    return newComputation;
}

template<typename T>
FixpointComputation<T> operator/(const FixpointComputation<T>& lhs, const Fixpoint<T>& rhs)
{
    auto newComputation = FixpointComputation<T>();
    newComputation.children.push_back(lhs);
    newComputation.children.push_back(rhs);
    newComputation.operation = FixpointOperation::division;
    return newComputation;
}

template<typename T>
FixpointComputation<T> operator/(Fixpoint<T>& lhs, const FixpointComputation<T>& rhs)
{
    auto newComputation = FixpointComputation<T>();
    newComputation.children.push_back(lhs);
    newComputation.children.push_back(rhs);
    newComputation.operation = FixpointOperation::division;
    return newComputation;
}

template<typename T>
FixpointComputation<T> operator/(Fixpoint<T>& lhs, const T& rhs)
{
    auto newComputation = FixpointComputation<T>();
    newComputation.children.push_back(lhs);
    newComputation.children.push_back(rhs);
    newComputation.operation = FixpointOperation::division;
    return newComputation;
}

template<typename T>
FixpointComputation<T> operator/(const T& lhs, Fixpoint<T>& rhs)
{
    auto newComputation = FixpointComputation<T>();
    newComputation.children.push_back(lhs);
    newComputation.children.push_back(rhs);
    newComputation.operation = FixpointOperation::division;
    return newComputation;
}

template<typename T>
FixpointComputation<T> operator+(const FixpointComputation<T>& lhs, const FixpointComputation<T>& rhs)
{
    auto newComputation = FixpointComputation<T>();
    newComputation.children.push_back(lhs);
    newComputation.children.push_back(rhs);
    newComputation.operation = FixpointOperation::addition;
    return newComputation;
}

template<typename T>
FixpointComputation<T> operator+(const FixpointComputation<T>& lhs, const FixpointReference<T>& rhs)
{
    auto newComputation = FixpointComputation<T>();
    newComputation.children.push_back(lhs);
    newComputation.children.push_back(rhs);
    newComputation.operation = FixpointOperation::addition;
    return newComputation;
}

template<typename T>
FixpointComputation<T> operator+(const FixpointReference<T>& lhs, const FixpointComputation<T>& rhs)
{
    auto newComputation = FixpointComputation<T>();
    newComputation.children.push_back(lhs);
    newComputation.children.push_back(rhs);
    newComputation.operation = FixpointOperation::addition;
    return newComputation;
}

template<typename T>
FixpointComputation<T> operator+(const FixpointReference<T>& lhs, const FixpointReference<T>& rhs)
{
    auto newComputation = FixpointComputation<T>();
    newComputation.children.push_back(lhs);
    newComputation.children.push_back(rhs);
    newComputation.operation = FixpointOperation::addition;
    return newComputation;
}

template<typename T>
FixpointComputation<T> operator+(const FixpointReference<T>& lhs, Fixpoint<T>& rhs)
{
    auto newComputation = FixpointComputation<T>();
    newComputation.children.push_back(lhs);
    newComputation.children.push_back(rhs);
    newComputation.operation = FixpointOperation::addition;
    return newComputation;
}

template<typename T>
FixpointComputation<T> operator+(Fixpoint<T>& lhs, const FixpointReference<T>& rhs)
{
    auto newComputation = FixpointComputation<T>();
    newComputation.children.push_back(lhs);
    newComputation.children.push_back(rhs);
    newComputation.operation = FixpointOperation::addition;
    return newComputation;
}

template<typename T>
FixpointComputation<T> operator+(const FixpointComputation<T>& lhs, Fixpoint<T>& rhs)
{
    auto newComputation = FixpointComputation<T>();
    newComputation.children.push_back(lhs);
    newComputation.children.push_back(rhs);
    newComputation.operation = FixpointOperation::addition;
    return newComputation;
}

template<typename T>
FixpointComputation<T> operator+(Fixpoint<T>& lhs, const FixpointComputation<T>& rhs)
{
    auto newComputation = FixpointComputation<T>();
    newComputation.children.push_back(lhs);
    newComputation.children.push_back(rhs);
    newComputation.operation = FixpointOperation::addition;
    return newComputation;
}

template<typename T>
FixpointComputation<T> operator+(Fixpoint<T>& lhs, const T& rhs)
{
    auto newComputation = FixpointComputation<T>();
    newComputation.children.push_back(lhs);
    newComputation.children.push_back(rhs);
    newComputation.operation = FixpointOperation::addition;
    return newComputation;
}

template<typename T>
FixpointComputation<T> operator+(const T& lhs, Fixpoint<T>& rhs)
{
    auto newComputation = FixpointComputation<T>();
    newComputation.children.push_back(lhs);
    newComputation.children.push_back(rhs);
    newComputation.operation = FixpointOperation::addition;
    return newComputation;
}

template<typename T>
FixpointComputation<T> operator+(const FixpointComputation<T>& lhs, const T& rhs)
{
    auto newComputation = FixpointComputation<T>();
    newComputation.children.push_back(lhs);
    newComputation.children.push_back(rhs);
    newComputation.operation = FixpointOperation::addition;
    return newComputation;
}

template<typename T>
FixpointComputation<T> operator+(const T& lhs, const FixpointComputation<T>& rhs)
{
    auto newComputation = FixpointComputation<T>();
    newComputation.children.push_back(lhs);
    newComputation.children.push_back(rhs);
    newComputation.operation = FixpointOperation::addition;
    return newComputation;
}

template<typename T>
FixpointComputation<T> operator*(const FixpointComputation<T>& lhs, const FixpointComputation<T>& rhs)
{
    auto newComputation = FixpointComputation<T>();
    newComputation.children.push_back(lhs);
    newComputation.children.push_back(rhs);
    newComputation.operation = FixpointOperation::multiplication;
    return newComputation;
}

template<typename T>
FixpointComputation<T> operator*(const FixpointComputation<T>& lhs, const FixpointReference<T>& rhs)
{
    auto newComputation = FixpointComputation<T>();
    newComputation.children.push_back(lhs);
    newComputation.children.push_back(rhs);
    newComputation.operation = FixpointOperation::multiplication;
    return newComputation;
}

template<typename T>
FixpointComputation<T> operator*(const FixpointReference<T>& lhs, const FixpointComputation<T>& rhs)
{
    auto newComputation = FixpointComputation<T>();
    newComputation.children.push_back(lhs);
    newComputation.children.push_back(rhs);
    newComputation.operation = FixpointOperation::multiplication;
    return newComputation;
}

template<typename T>
FixpointComputation<T> operator*(const FixpointReference<T>& lhs, const FixpointReference<T>& rhs)
{
    auto newComputation = FixpointComputation<T>();
    newComputation.children.push_back(lhs);
    newComputation.children.push_back(rhs);
    newComputation.operation = FixpointOperation::multiplication;
    return newComputation;
}

template<typename T>
FixpointComputation<T> operator*(const FixpointReference<T>& lhs, Fixpoint<T>& rhs)
{
    auto newComputation = FixpointComputation<T>();
    newComputation.children.push_back(lhs);
    newComputation.children.push_back(rhs);
    newComputation.operation = FixpointOperation::multiplication;
    return newComputation;
}

template<typename T>
FixpointComputation<T> operator*(Fixpoint<T>& lhs, const FixpointReference<T>& rhs)
{
    auto newComputation = FixpointComputation<T>();
    newComputation.children.push_back(lhs);
    newComputation.children.push_back(rhs);
    newComputation.operation = FixpointOperation::multiplication;
    return newComputation;
}

template<typename T>
FixpointComputation<T> operator*(const FixpointComputation<T>& lhs, Fixpoint<T>& rhs)
{
    auto newComputation = FixpointComputation<T>();
    newComputation.children.push_back(lhs);
    newComputation.children.push_back(rhs);
    newComputation.operation = FixpointOperation::multiplication;
    return newComputation;
}

template<typename T>
FixpointComputation<T> operator*(Fixpoint<T>& lhs, const FixpointComputation<T>& rhs)
{
    auto newComputation = FixpointComputation<T>();
    newComputation.children.push_back(lhs);
    newComputation.children.push_back(rhs);
    newComputation.operation = FixpointOperation::multiplication;
    return newComputation;
}

template<typename T>
FixpointComputation<T> operator*(Fixpoint<T>& lhs, const T& rhs)
{
    auto newComputation = FixpointComputation<T>();
    newComputation.children.push_back(lhs);
    newComputation.children.push_back(FixpointReference<T>(rhs));
    newComputation.operation = FixpointOperation::multiplication;
    return newComputation;
}

template<typename T>
FixpointComputation<T> operator*(const T& lhs, Fixpoint<T>& rhs)
{
    auto newComputation = FixpointComputation<T>();
    newComputation.children.push_back(lhs);
    newComputation.children.push_back(rhs);
    newComputation.operation = FixpointOperation::multiplication;
    return newComputation;
}

template<typename T>
FixpointComputation<T> FixpointParameterComputation<T>::operator=(const FixpointComputation<T>& rhs)
{
    auto newComputation = std::make_unique<FixpointComputation<T>>();
    newComputation->children.push_back(*this);
    newComputation->children.push_back(rhs);
    newComputation->operation = FixpointOperation::parametrized_equivalence;
    auto newComputationPtr = newComputation.get();
    std::get<Fixpoint<T>*>(std::get<FixpointReference<T>>(this->children[0]).value)->fixpointComputations.push_back(std::move(newComputation));
    return *newComputationPtr;
}

template<typename T>
FixpointComputation<T> FixpointParameterComputation<T>::operator=(const T& rhs)
{
    auto newComputation = std::make_unique<FixpointComputation<T>>();
    newComputation->children.push_back(*this);
    newComputation->children.push_back(FixpointReference<T>(rhs));
    newComputation->operation = FixpointOperation::parametrized_equivalence;
    auto newComputationPtr = newComputation.get();
    std::get<Fixpoint<T>*>(std::get<FixpointReference<T>>(this->children[0]).value)->fixpointComputations.push_back(std::move(newComputation));
    return *newComputationPtr;
}

template<typename T>
FixpointComputation<T> FixpointParameterComputation<T>::operator=(const FixpointReference<T>& rhs)
{
    auto newComputation = std::make_unique<FixpointComputation<T>>();
    newComputation->children.push_back(*this);
    newComputation->children.push_back(rhs);
    newComputation->operation = FixpointOperation::parametrized_equivalence;
    auto newComputationPtr = newComputation.get();
    std::get<Fixpoint<T>*>(std::get<FixpointReference<T>>(this->children[0]).value)->fixpointComputations.push_back(std::move(newComputation));
    return *newComputationPtr;
}

template<typename T>
FixpointComputation<T> FixpointParameterComputation<T>::operator=(Fixpoint<T>& rhs)
{
    auto newComputation = std::make_unique<FixpointComputation<T>>();
    newComputation->children.push_back(*this);
    newComputation->children.push_back(rhs);
    newComputation->operation = FixpointOperation::parametrized_equivalence;
    auto newComputationPtr = newComputation.get();
    std::get<Fixpoint<T>*>(std::get<FixpointReference<T>>(this->children[0]).value)->fixpointComputations.push_back(std::move(newComputation));
    return *newComputationPtr;
}

template<typename T>
FixpointComputation<T> Fixpoint<T>::operator=(const FixpointComputation<T>& rhs)
{
    auto newComputation = FixpointComputation<T>();
    newComputation.children.push_back(this);
    newComputation.children.push_back(rhs);
    newComputation.operation = FixpointOperation::next_layer_equivalence;
    return newComputation;
}


template<typename T>
FixpointParameterComputation<T> Fixpoint<T>::operator()(FixpointParameter parameter)
{
    auto newComputation = FixpointParameterComputation<T>();
    newComputation.children.push_back(this);
    newComputation.children.push_back(parameter);
    newComputation.operation = FixpointOperation::parametrized_reference;
    return newComputation;
}

template<typename T>
struct FixpointSpecialCeil
{
public:
    FixpointComputation<T> value;

public:
    FixpointSpecialCeil(const FixpointComputation<T>& value_)
        : value(value_)
    {
    }
};

template<typename T>
FixpointComputation<T> operator*(const FixpointComputation<T>& lhs, const FixpointParameter& rhs)
{
    auto newComputation = FixpointComputation<T>();
    newComputation.children.push_back(lhs);
    newComputation.children.push_back(rhs);
    newComputation.operation = FixpointOperation::multiplication;
    return newComputation;
}

template<typename T>
FixpointComputation<T> operator*(const FixpointSpecialCeil<T>& lhs, const FixpointReference<T>& rhs)
{
    auto newComputation = FixpointComputation<T>();
    newComputation.children.push_back(lhs.value);
    newComputation.operation = FixpointOperation::ceil;

    auto newNewComputation = FixpointComputation<T>();
    newNewComputation.children.push_back(newComputation);
    newNewComputation.children.push_back(rhs);
    newNewComputation.operation = FixpointOperation::multiplication;
    return newNewComputation;
}

template<typename T>
FixpointComputation<T> operator*(const FixpointSpecialCeil<T>& lhs, const T& rhs)
{
    auto newComputation = FixpointComputation<T>();
    newComputation.children.push_back(lhs.value);
    newComputation.operation = FixpointOperation::ceil;

    auto newNewComputation = FixpointComputation<T>();
    newNewComputation.children.push_back(newComputation);
    newNewComputation.children.push_back(rhs);
    newNewComputation.operation = FixpointOperation::multiplication;
    return newNewComputation;
}

#endif // DEAMER_FP_H