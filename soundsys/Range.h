#pragma once

template <class T>
class Range {
public:
	Range(const T &begin = 0, const T &len = 0)
		: m_begin(begin)
		, m_len(len)
	{}

	T length() const { return m_len; }
	T begin() const { return m_begin; }
	T end() const { return m_begin + m_len; }

	bool empty() const { return begin() >= end(); }

private:
	T	m_begin;
	T	m_len;
};