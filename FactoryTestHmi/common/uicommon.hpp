//
// Created by panshiquan on 2026/5/10.
//

#ifndef FACTORYTESTMODULE_HMI_COMMON_H
#define FACTORYTESTMODULE_HMI_COMMON_H

#define AUTO_PROPERTY(Type, Name) \
public: \
    Q_PROPERTY(Type Name READ Name WRITE set_##Name NOTIFY Name##Changed) \
    Type Name() const { return m_##Name; } \
    void set_##Name(Type value) { \
        if (m_##Name != value) { \
            m_##Name = value; \
            Q_EMIT Name##Changed(); \
        } \
    } \
Q_SIGNALS: \
    void Name##Changed(); \
private: \
    Type m_##Name;


// 只读版本（没有 setter 和 NOTIFY，适合常量状态）
#define READONLY_PROPERTY(Type, Name) \
public: \
    Q_PROPERTY(Type Name READ Name CONSTANT) \
    Type Name() const { return m_##Name; } \
private: \
    Type m_##Name;


#endif //FACTORYTESTMODULE_COMMON_H
