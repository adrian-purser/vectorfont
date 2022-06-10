//=============================================================================
//	FILE:					xml.h
//	SYSTEM:				AdeXML
//	DESCRIPTION:	XML Document Parser.
//								This is a lightweight XML document parser. This is far from
//								being standards compliant and is intended as a quick and 
//								easy method of parsing XML documents with a focus on 
//								configuration files. 
//-----------------------------------------------------------------------------
//  COPYRIGHT:		(C) Copyright 2008-2017 Adrian Purser. All Rights Reserved.
//	LICENCE:			MIT - See LICENSE file for details
//	MAINTAINER:		Adrian Purser <ade@adrianpurser.co.uk>
//	CREATED:			06-NOV-2008 Adrian Purser <ade@adrianpurser.co.uk>
//-----------------------------------------------------------------------------
//	TODO :
//	* This is old code and due for a review.
//	*	The 'Processing Instruction' ( <? ) parser works with well formed xml 
//		but could fail, or even hang, with malformed xml.
//
//=============================================================================
#ifndef GUARD_ADE_XML_H
#define GUARD_ADE_XML_H

#include <cstdlib>
#include <cstdint>
#include <string>
#include <map>
#include <vector>
#include <cassert>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <functional>
#include <memory>

namespace ade { namespace xml 
{

//typedef std::vector<class XMLNode *>	node_array_t;
//typedef std::vector<class XMLElement *>	element_array_t;

//=============================================================================
//
//	PARSER
//
//=============================================================================

class XMLParser
{
public:
	struct token_t
	{
		enum type_t
		{
			UNKNOWN,
			STRING,
			TEXT,
			WHITE_SPACE,
			EQUALS,
			END_ELEMENT,
			CLOSE_ELEMENT,
			START_ELEMENT,
			FORWARD_SLASH,
			START_COMMENT,
			END_COMMENT,
			START_QUESTION_MARK,
			END_QUESTION_MARK,
			START_DECLARATION,
			OPEN_SQUARE_BRACKET,
			CLOSE_SQUARE_BRACKET
		};
		
		type_t			type;
		std::string		text;
	};

	enum Encoding 
	{
		ENCODING_PLAIN_TEXT,
		ENCODING_UTF8,
		ENCODING_UTF16_LITTLE_ENDIAN,
		ENCODING_UTF16_BIG_ENDIAN,
		ENCODING_UTF32_LITTLE_ENDIAN,
		ENCODING_UTF32_BIG_ENDIAN
	};

	typedef std::vector<token_t>									token_array_t;
	typedef std::vector<token_t>::iterator				iterator;
	typedef std::vector<token_t>::const_iterator	const_iterator;

private:
	std::uint8_t *												m_p_buffer;
	std::uint8_t *												m_p_buffer_end;
	std::uint8_t *												m_p_current_position;
	std::map<std::string,unsigned long>		m_char_entities;
	token_array_t													m_tokens;
	Encoding															m_encoding = ENCODING_PLAIN_TEXT;

private:
	std::uint32_t read_char(size_t offset = 0,bool b_peek = false)
	{
		const size_t avail = m_p_buffer_end-m_p_current_position;
		if(!avail)
			return 0;

		std::uint32_t ch = 0;
		size_t size = 1;

		switch(m_encoding)
		{
			case ENCODING_PLAIN_TEXT :	
			case ENCODING_UTF8 :					
				ch = m_p_current_position[offset]; 
				break;

			case ENCODING_UTF16_LITTLE_ENDIAN :		
				if(avail>=2)
				{
					size = 2;
					ch = m_p_current_position[offset*size] | (m_p_current_position[(offset*size)+1]<<8);
				}
				break;

			case ENCODING_UTF16_BIG_ENDIAN :		
				if(avail>=2)
				{
					size = 2;
					ch = m_p_current_position[(offset*size)+1] | (m_p_current_position[offset*size]<<8);
				}
				break;

			case ENCODING_UTF32_LITTLE_ENDIAN :		
				if(avail>=4)
				{
					size = 4;
					ch = m_p_current_position[offset*size] | 
						(m_p_current_position[(offset*size)+1]<<8) |
						(m_p_current_position[(offset*size)+2]<<16) |
						(m_p_current_position[(offset*size)+3]<<24);
				}
				break;

			case ENCODING_UTF32_BIG_ENDIAN :
				if(avail>=4)
				{
					size = 4;
					ch = m_p_current_position[(offset*size)+3] | 
						(m_p_current_position[(offset*size)+2]<<8) |
						(m_p_current_position[(offset*size)+1]<<16) |
						(m_p_current_position[offset*size]<<24);
				}
				break;

			default :	break;
		}

		if(!b_peek)
			m_p_current_position+=((offset+1)*size);

		return ch;
	}

	std::uint32_t peek_char(size_t offset = 0) {return read_char(offset,true);}

	void reset_char_entities()
	{
		m_char_entities.clear();
		m_char_entities["quot"]		= 0x0022;
		m_char_entities["amp"]		= 0x0026;
		m_char_entities["apos"]		= 0x0027;
		m_char_entities["lt"]			= 0x003C;
		m_char_entities["gt"]			= 0x003E;
		m_char_entities["#163"]		= 0x00A3;
		m_char_entities["euro"]		= 0x0080;
	}

	//-------------------------------------------------------------------------
	//	PARSE TOKEN
	//-------------------------------------------------------------------------
	int	parse_tokens(token_array_t & tokens)
	{
		token_t tok;
		bool finished = false;
		bool err = false;

		while(!finished && !err)
		{
			auto ch = peek_char();
			if(!ch)
				break;
		
			tok.text.clear();
			tok.type = token_t::UNKNOWN;

			switch(ch)
			{
				//---------------------------------------------------------------------
				case	32 :
				case	9 :
				case	13 :
				case	10 :
				case	12 :
				//---------------------------------------------------------------------
					tok.type = token_t::WHITE_SPACE;
					for(;;)
					{
						auto wch = peek_char();
						if(	(wch == 32) ||
							(wch == 9)	||
							(wch == 10) ||
							(wch == 12) ||
							(wch == 13) )
							tok.text.push_back(static_cast<char>(read_char()));
						else
							break;
					}
					break;

				//---------------------------------------------------------------------
				case	'<' : 
				//---------------------------------------------------------------------
					{
						read_char();
						ch = peek_char();

						if(	(ch=='!') &&
							(peek_char(1)=='-') &&
							(peek_char(2)=='-') )
						{
							read_char(2);
							tok.type = token_t::START_COMMENT;
							tokens.push_back(tok);			
							for(;;)
							{
								ch = read_char();
								if(ch)
								{
									if((ch == '-') && (peek_char(0)=='-') && (peek_char(1)=='>'))
									{
										read_char(1);
										if(!tok.text.empty())
										{
											tok.type = token_t::TEXT;
											tokens.push_back(tok);
										}
										tok.type = token_t::END_COMMENT;
										tok.text.clear();
										tokens.push_back(tok);
										break;
									}
									else
										tok.text.push_back(static_cast<char>(ch));
								}
							}
						}
						else if(ch=='?')
						{
							tok.type = token_t::START_QUESTION_MARK;
							read_char();
						}
						else if(ch=='!')
						{
							tok.type = token_t::START_DECLARATION;
							read_char();
						}
						else
							tok.type = token_t::START_ELEMENT;
					}
					break;

				//---------------------------------------------------------------------
				case	'>' : 
				//---------------------------------------------------------------------
					tok.type = token_t::END_ELEMENT;
					read_char();
					break;

				//---------------------------------------------------------------------
				case	'[' :
				//---------------------------------------------------------------------
					tok.type = token_t::OPEN_SQUARE_BRACKET;
					read_char();
					break;

				//---------------------------------------------------------------------
				case	']' :
				//---------------------------------------------------------------------
					tok.type = token_t::CLOSE_SQUARE_BRACKET;
					read_char();
					break;

				//---------------------------------------------------------------------
				case	'=' :
				//---------------------------------------------------------------------
					tok.type = token_t::EQUALS;
					read_char();
					break;

				//---------------------------------------------------------------------
				case	'/' :
				//---------------------------------------------------------------------
					read_char();
					if(peek_char()=='>')
					{
						tok.type = token_t::CLOSE_ELEMENT;
						read_char();
					}
					else
						tok.type = token_t::FORWARD_SLASH;
					break;

				//---------------------------------------------------------------------
				default :
				//---------------------------------------------------------------------
					if(	(ch=='-') &&
						(peek_char(1)=='-') &&
						(peek_char(2)=='>') )
					{
						tok.type = token_t::END_COMMENT;
						read_char(2);
					}
					else if((ch=='?') &&
							(peek_char(1)=='>'))
					{
						tok.type = token_t::END_QUESTION_MARK;
						read_char(1);
					}
					else
					{
						tok.type	= token_t::STRING;
						err			= !!parse_string(tok.text);
					}
					break;
			}

			if(tok.type != token_t::UNKNOWN)
			{
				tokens.push_back(tok);

				if(	(tok.type==token_t::END_ELEMENT) ||
					(tok.type==token_t::END_COMMENT) ||
					(tok.type==token_t::END_QUESTION_MARK) ||
					(tok.type==token_t::CLOSE_ELEMENT) )
				{
					tok.text.clear();
					bool whitespace = true;
					while((m_p_current_position<m_p_buffer_end) && (peek_char()!='<'))
					{
						ch = parse_char();
						switch(ch)
						{
							case 0 : break;
							default : whitespace = false; [[fallthrough]];
							case 32 : case 9 : case 13 : case 10 : case 12 : 
								tok.text.push_back(static_cast<char>(ch));
								break;
						}
						
					}
					tok.type = (whitespace ? token_t::WHITE_SPACE : token_t::TEXT);
					tokens.push_back(tok);
				}
			}
		}

		return (err ? -1:0);
	}

	//-------------------------------------------------------------------------
	//	PARSE CHAR
	//-------------------------------------------------------------------------
	std::uint32_t parse_char()
	{
		if(m_p_current_position >= m_p_buffer_end)
			return 0;

		auto ch = read_char();
		if(ch != '&')
			return ch;

		std::string entity;
		
		for(;;)
		{
			ch = peek_char();
			if(ch && (ch!=';') && isalnum(ch))
			{
				entity.push_back(static_cast<char>(read_char()));
				continue;
			}

			break;
		}

		if(ch == ';')
			read_char();

		unsigned long result = 0;

		if(m_p_current_position<m_p_buffer_end)
		{
			auto ifind = m_char_entities.find(entity);

			if(ifind!=m_char_entities.end())
				result = ifind->second;
		}

		return result;
	}

	//-------------------------------------------------------------------------
	//	PARSE STRING
	//-------------------------------------------------------------------------
	int	parse_string(std::string & result)
	{
		bool quotes = false;
		bool finished = false;

		result.clear();

		if(peek_char()==34)
		{
			read_char();
			quotes=true;
		}

		while(!finished && (m_p_current_position<m_p_buffer_end))
		{
			if(quotes)
			{
				auto ch = peek_char();
				switch(ch)
				{
					//-------------------------------------------------------------
					case	8 :		// Skip Control Characters.
					case	10 :
					case	11 :
					case	13 :
					//-------------------------------------------------------------
						read_char();
						break;

					//-------------------------------------------------------------
					case	9 :
					//-------------------------------------------------------------
						result.push_back(static_cast<char>(read_char()));
						break;

					//-------------------------------------------------------------
					case	34 :
					//-------------------------------------------------------------
						finished=true;
						read_char();
						break;

					//-------------------------------------------------------------
					default :
					//-------------------------------------------------------------
						if(ch >= 32)
							result.push_back((char)parse_char());
						else
							read_char();
						break;
				}
			}
			else
			{
				auto ch = peek_char();

				switch(ch)
				{
					//-------------------------------------------------------------
					case	8 :		// Skip Control Characters.
					//-------------------------------------------------------------
						read_char();
						break;

					//-------------------------------------------------------------
					case	'=' :
					case	'>' :
					case	'/' :
					case	9 :
					case	32 :
					case	10 :
					case	11 :
					case	12 :
					case	13 :
					//-------------------------------------------------------------
						finished=true;
						break;

					//-------------------------------------------------------------
					case	'&' :
					default :
					//-------------------------------------------------------------
						if(ch >= 32)
							result.push_back((char)parse_char());
						else
							read_char();
						break;
				}
			}
		}

		return (m_p_current_position<m_p_buffer_end ? 0:-1);
	}

	void read_byte_order_mark()
	{
		size_t avail = m_p_buffer_end-m_p_buffer;
		m_encoding = ENCODING_PLAIN_TEXT;

		if(	(avail>=3) &&
			(m_p_buffer[0] == 0x0EF) &&
			(m_p_buffer[1] == 0x0BB) &&
			(m_p_buffer[2] == 0x0BF) )
		{
			m_encoding = ENCODING_UTF8;
			m_p_buffer += 3;
		}
		else
		{
			std::uint32_t bom = (m_p_buffer[0] << 24) |
								(m_p_buffer[1] << 16) |
								(m_p_buffer[2] << 8) |
								(m_p_buffer[3]);

			if(bom == 0x00000FEFF)
			{
				m_encoding = ENCODING_UTF32_BIG_ENDIAN;
				m_p_buffer += 4;
			}
			else if(bom == 0x0FFFE0000)
			{
				m_encoding = ENCODING_UTF32_LITTLE_ENDIAN;
				m_p_buffer += 4;
			}
			else
			{
				bom >>= 16;
				if(bom == 0x0FEFF)
				{
					m_encoding = ENCODING_UTF16_BIG_ENDIAN;
					m_p_buffer += 2;
				}
				else if(bom == 0x0FFFE)
				{
					m_encoding = ENCODING_UTF16_LITTLE_ENDIAN;
					m_p_buffer += 2;
				}
			}
		}

		m_p_current_position = m_p_buffer;
	}

public:
	XMLParser(void) = delete;
	XMLParser( const XMLParser& ) = delete;
	const XMLParser& operator=( const XMLParser& ) = delete;

	XMLParser(char * pbuffer,size_t buffer_size) :
		m_p_buffer((std::uint8_t *)pbuffer),
		m_p_buffer_end((std::uint8_t *)(pbuffer+buffer_size)),
		m_p_current_position((std::uint8_t *)pbuffer)
	{
		assert(pbuffer);
		assert(m_p_buffer_end);
	
		reset_char_entities();
		read_byte_order_mark();

		token_t	token;

		parse_tokens(m_tokens);
	}

	~XMLParser(void)
	{}

	int	count() const {return (int)m_tokens.size();}
	bool empty() const {return m_tokens.empty();}

	iterator		begin()			{return m_tokens.begin();}
	const_iterator	begin() const	{return m_tokens.begin();}

	iterator		end()			{return m_tokens.end();}
	const_iterator	end() const		{return m_tokens.end();}

	token_t & operator[](int index) {return m_tokens.at(index);}

//	char * current_pointer() {return (char *)m_p_current_position;}
};


//=============================================================================
//
//	RESOURCE FACTORY
//
//=============================================================================

struct FileData
{
	std::shared_ptr<const char>		p_data;
	size_t							datasize;
};

class IResourceFactory
{
public:
	virtual FileData	load(const std::string & uri) = 0;
	virtual FileData	load_public(const std::string & public_id) = 0;
};


//=============================================================================
//
//	DOCTYPE
//
//=============================================================================

class DTDContentModelNode
{
private:
	std::string							m_name;
	char								m_modifier = 0;
	char								m_seq_type = 0;
	std::vector<DTDContentModelNode>	m_children;

public:
	bool operator==(const std::string & other) const	{return m_name == other;}
	bool is_leaf() const								{return !m_name.empty();}
	void set_name(const std::string & name)				{m_name = name;}
	void set_modifier(char modifier)					{m_modifier = modifier;}
	char get_modifier() const							{return m_modifier;}
	void set_sequence_type(char type)					{m_seq_type = type;}
	char get_sequence_type() const						{return m_seq_type;}

	void push_back(DTDContentModelNode && node)			{m_children.push_back(std::forward<DTDContentModelNode>(node));}

	char get_modifier(const std::string & name) const
	{
		if(m_name == name)
			return m_modifier;

		for(const auto & node : m_children)
		{
			auto mod = node.get_modifier(name);
			if(mod>=0)
				return mod;
		}

		return -1;
	}

	bool is_array(const std::string & name) const
	{
		bool b_is_array_modifier = ((m_modifier == '*') || (m_modifier == '+'));
		bool b_is_array = false;

		if(m_name == name)
			return b_is_array_modifier;

		if(/*m_seq_type == '|' && */ b_is_array_modifier)
		{
			for(const auto & node : m_children)
				if(node.get_modifier(name) >= 0)
				{
					b_is_array = true;
					break;
				}
		}
		else
		{
			for(const auto & node : m_children)
				if(node.is_array(name))
				{
					b_is_array = true;
					break;
				}
		}

		return b_is_array;
	}


};

class DocTypeElement
{
private:
	std::string						m_name;
	DTDContentModelNode		m_root;

public:
	bool operator==(const std::string & other) const {return m_name == other;}

	void					set_name(const std::string & name)	{m_name = name;}
	const std::string &		get_name() const					{return m_name;}

	char get_modifier(const std::string & name)	{return m_root.get_modifier(name);}
	bool is_array(const std::string & name) const {return m_root.is_array(name);}

	int parse_content_model(const std::string & content)
	{
		if((content.size() < 3) || (content[0] != '(') || (content.back() != ')'))
			return -1;

		return parse_content_model(++(std::begin(content)),std::end(content),m_root);
	}

private:
	int
	parse_content_model(std::string::const_iterator	&	ib,
						std::string::const_iterator		ie,
						ade::xml::DTDContentModelNode & out_node )
	{
		bool b_err					= false;
		bool b_finished_subobject	= false;

		while((ib!=ie) && !b_err && !b_finished_subobject)
		{
			if(*ib == '(')
			{
				++ib;
				ade::xml::DTDContentModelNode node;
				if(parse_content_model(ib,ie,node))
					b_err = true;
				else
				{
					switch(*ib)
					{
						case '+' :
						case '*' :
						case '?' :
							node.set_modifier(*ib);
							++ib;
							break;

						default : break;
					}
					out_node.push_back(std::move(node));
				}
			}
			else
			{
				std::string name;
				bool b_finished = false;
				char modifier = 0;

				while(!b_finished && (ib!=ie))
				{
					auto ch = *ib++;
					switch(ch)
					{
						case '+' :
						case '*' :
						case '?' :
							modifier = ch;
							break;

						case ',' :
						case '|' :
							out_node.set_sequence_type(ch); 
							[[fallthrough]];
						case ')' :
							b_finished = true;
							if(!name.empty())
							{
								ade::xml::DTDContentModelNode node;
								node.set_name(name);
								node.set_modifier(modifier);
								out_node.push_back(std::move(node));	
							}

							b_finished_subobject = !!(ch == ')');
							break;

						default :
							if(modifier)
							{
								name.push_back(modifier);
								modifier = 0;
							}
							name.push_back(ch);
							break;
					}
				}
			}

		}

		return b_err ? -1 : 0;
	}


};

class DocType
{
private:
	std::string						m_name;
	std::string						m_public_id;
	std::string						m_system_id;
	std::vector<DocTypeElement>		m_elements;

public:
	void							set_name(const std::string & name)		{m_name = name;}
	void							set_public_id(const std::string & id)	{m_public_id = id;}
	void							set_system_id(const std::string & id)	{m_system_id = id;}

	void add_element(DocTypeElement && element)
	{
		const auto & name = element.get_name();
		auto ifind = std::find(std::begin(m_elements),std::end(m_elements),name);

		if(ifind == std::end(m_elements))
			m_elements.push_back(std::forward<DocTypeElement>(element));
		else
			*ifind = std::move(element);
	}

	bool is_element_an_array(const std::string & parent_name,const std::string & element_name) const
	{
		auto ifind = std::find(std::begin(m_elements),std::end(m_elements),parent_name);

		if(ifind == std::end(m_elements))
			return false;

		return ifind->is_array(element_name);
	}

};



//=============================================================================
//
//	NODE
//
//=============================================================================

class XMLElement;

class XMLNode
{
public:
	typedef enum
	{
		DECLARATION,
		ELEMENT,
		TEXT,
		COMMENT
	} type_t;

	virtual type_t				type() const = 0;
	virtual	const std::string & value() const = 0;
	virtual void				find_children(const std::string & name,std::function<bool(const XMLNode *)> ) const = 0;
	virtual void				find_elements(const std::string & name,std::function<bool(const XMLElement *)> ) const = 0;
	virtual void				write(std::ostream & os,int indent = -1) const = 0;

	XMLNode() {}
	virtual ~XMLNode() {}

};

//=============================================================================
//
//	TEXT
//
//=============================================================================

class XMLText : public XMLNode
{
private:
	std::string		m_text;

public:
	XMLText(const std::string & text) :	m_text(text){}
	virtual ~XMLText() {}

	const std::string & value() const override {return m_text;}
	type_t				type() const override {return TEXT;}
	void				find_children(const std::string & /*name*/,std::function<bool(const XMLNode *)>) const override {}
	void				find_elements(const std::string & /*name*/,std::function<bool(const XMLElement *)>) const override {}
	void				write(std::ostream & os,int /*indent*/) const override	{os << m_text << "\n";}
};

//=============================================================================
//
//	ELEMENT
//
//=============================================================================

class XMLElement : public XMLNode
{
private:
	std::string									m_name;
	std::vector<std::unique_ptr<XMLNode>>		m_children;
	std::map<std::string,std::string>			m_attributes;

private:
	long stringtolong(const char * str,size_t size) const
	{
		long lval = 0;
		if(str)
		{
			if(	((*str=='0') && (str[1]=='x')) ||
				((*str=='U') && (str[1]=='+')))
			{
				unsigned char ch;
				for(size_t i=2;i<size;++i)
				{
					ch = static_cast<unsigned char>(toupper(str[i]));
					if(ch>='0' && ch<='9')		lval = (lval<<4) | (ch -= '0');
					else if(ch>='A' && ch<='F')	lval = (lval<<4) | (ch-('A'-10));
					else break;
				}
			}
			else if(*str=='#')
			{
				unsigned char ch;
				int sz=0;
				for(size_t i=1;i<size;++i)
				{
					ch = static_cast<unsigned char>(toupper(str[i]));
					if(ch>='0' && ch<='9')		{lval = (lval<<4) | (ch -= '0');++sz;}
					else if(ch>='A' && ch<='F')	{lval = (lval<<4) | (ch-('A'-10));++sz;}
					else break;
				}				

				switch(sz)
				{
					case	3 :		
						lval =  (((lval&0xF00)<<(3*4))|((lval&0xF00)<<(2*4))) | 
								(((lval&0x0F0)<<(2*4))|((lval&0x0F0)<<(1*4))) | 
								(((lval&0x00F)<<(1*4))|(lval&0x00F)) | 
								0x0FF000000;
								break;

					case	4 :
						lval =  (((lval&0xF000)<<(4*4))|((lval&0xF000)<<(3*4))) | 
								(((lval&0x0F00)<<(3*4))|((lval&0x0F00)<<(2*4))) | 
								(((lval&0x00F0)<<(2*4))|((lval&0x00F0)<<(1*4))) | 
								(((lval&0x000F)<<(1*4))|(lval&0x000F));
								break;

					case	6 :
						lval |= 0x0FF000000;
						break;
				};
			}
			else 
				lval = atol(str);
		}
		return lval;
	}

public:
	XMLElement(const std::string & element_name) :
		m_name(element_name)
		{}

	virtual ~XMLElement()
	{
		m_children.clear();	
	}

	const std::string & value() const override	{return m_name;}
	type_t				type() const override	{return ELEMENT;}

	void		push_back(std::unique_ptr<XMLNode> && p_node)	{m_children.push_back(std::forward<std::unique_ptr<XMLNode>>(p_node));}

	std::string escape_string(const char * str,size_t size) const
	{
		std::string escstr;

		for(const char * p_str = str;size;--size,++p_str)
		{
			switch((unsigned char)*p_str)
			{
				case 0 :		size = 0; break;
				case 0x022 :	escstr.append("&quot;"); break;
				case 0x026 :	escstr.append("&amp;"); break;
				case 0x027 :	escstr.append("&apos;"); break;
				case 0x03C :	escstr.append("&lt;"); break;
				case 0x03E :	escstr.append("&gt;"); break;
				case 0x0A3 :	escstr.append("&#163;"); break;
				case 0x080 :	escstr.append("&euro;"); break;
				default : escstr.push_back(*p_str); break;
			}
		}

		return escstr;
	}

	std::string escape_string(const std::string & str) const
	{
		return escape_string(str.c_str(),str.size());
	}

	void write(std::ostream & os,int indent) const override
	{
		if(!m_name.empty())
		{
			for(int i=indent;i>0;--i)	os << '\t';
			os << "<" << escape_string(m_name);

			//-----------------------------------------------------------------
			//	Write the Attributes.
			//-----------------------------------------------------------------
			for(auto & attr_pair : m_attributes)
				os << " " << escape_string(attr_pair.first) << "=\"" << escape_string(attr_pair.second) << "\"";

			//-----------------------------------------------------------------
			//	Write the Child Nodes if there are any.
			//-----------------------------------------------------------------
			os << (m_children.empty() ? "/>\n" : ">\n");
		}
		else
			--indent;	

		if(!m_children.empty())
		{
			for(auto & p_child : m_children)
				p_child->write(os,(indent >= 0 ? indent+1 : 0));

			if(!m_name.empty())
			{
				for(int i=indent;i>0;--i) os << '\t';
				os << "</" << escape_string(m_name) << ">\n";
			}
		}
	}

	//----------------------------------------------------------------------------
	//	ATRRIBUTES
	//----------------------------------------------------------------------------
	template<typename T> void set_attribute(const std::string & name,T value)
	{
		m_attributes[name] = std::to_string(value);
	}

	void set_attribute(const std::string & name,const std::string & value)
	{
		m_attributes[name] = value;
	}

	bool get_attribute(const std::string & name,unsigned long & value) const
	{
		auto ifind = m_attributes.find(name);
		if(ifind==m_attributes.end())
			return false;
		value=stringtolong(ifind->second.c_str(),ifind->second.size());
		return true;
	}

	bool get_attribute(const std::string & name,long & value) const
	{
		auto ifind = m_attributes.find(name);
		if(ifind==m_attributes.end())
			return false;
		value=stringtolong(ifind->second.c_str(),ifind->second.size());
		return true;
	}

	bool get_attribute(const std::string & name,unsigned int & value) const
	{
		auto ifind = m_attributes.find(name);
		if(ifind==m_attributes.end())
			return false;
		value=(unsigned int)stringtolong(ifind->second.c_str(),ifind->second.size());
		return true;
	}

	bool get_attribute(const std::string & name,int & value) const
	{
		auto ifind = m_attributes.find(name);
		if(ifind==m_attributes.end())
			return false;
		value=(int)stringtolong(ifind->second.c_str(),ifind->second.size());
		return true;
	}

	bool get_attribute(const std::string & name,unsigned short & value) const
	{
		auto ifind = m_attributes.find(name);
		if(ifind==m_attributes.end())
			return false;
		value=static_cast<unsigned short>(stringtolong(ifind->second.c_str(),ifind->second.size()));
		return true;
	}

	bool get_attribute(const std::string & name,short & value) const
	{
		auto ifind = m_attributes.find(name);
		if(ifind==m_attributes.end())
			return false;
		value=static_cast<unsigned short>(stringtolong(ifind->second.c_str(),ifind->second.size()));
		return true;
	}

	bool get_attribute(const std::string & name,float & value) const
	{
		auto ifind = m_attributes.find(name);
		if(ifind==m_attributes.end())
			return false;
		value=(float)atof(ifind->second.c_str());
		return true;
	}

	bool get_attribute(const std::string & name,bool & value) const
	{
		auto ifind = m_attributes.find(name);
		if(ifind==m_attributes.end())
			return false;

		std::string boolval = ifind->second;
		std::transform(boolval.begin(),boolval.end(),boolval.begin(),::tolower);

		value = !!((boolval=="1") || (boolval=="true") || (boolval=="yes"));
		return true;
	}

	bool get_attribute(const std::string & name,std::string & out_attribute) const
	{
		auto ifind = m_attributes.find(name);	
		if(ifind == m_attributes.end())
			return false;
		out_attribute = ifind->second;
		return true;
	}

	void get_attributes(std::function<bool(const std::string & name,const std::string & value)> func) const 
	{
		for(auto & attrpair : m_attributes)
			if(func(attrpair.first,attrpair.second))
				break;
	}

	int attribute_count() const {return static_cast<int>(m_attributes.size());}

	//----------------------------------------------------------------------------
	//	NODE ACCESS
	//----------------------------------------------------------------------------
	const XMLNode * get_first(XMLNode::type_t type)
	{
		XMLNode * p_found = nullptr;

		for(auto & p_node : m_children)
			if(p_node->type() == type)
			{
				p_found = p_node.get();
				break;
			}

		return p_found;
	}

	const XMLElement * get_element(const std::string & id) const 
	{
		const XMLElement * p_found = nullptr;

		for(auto & p_node : m_children)
			if(	(p_node->type() == XMLNode::ELEMENT) &&
				(p_node->value() == id ) )
			{
				p_found=static_cast<const XMLElement *>(p_node.get());
				break;
			}

		return p_found;
	}

	int child_element_count() const
	{
		int count = 0;
		for(auto & p_node : m_children)
			if(p_node->type() == XMLNode::ELEMENT)
				++count;

		return count;
	}

	int non_empty_child_element_count() const
	{
		int count = 0;
		for(auto & p_node : m_children)
		{
			if(p_node->type() == XMLNode::ELEMENT)
			{
				const auto p_element = static_cast<XMLElement *>(p_node.get());
				if((p_element->attribute_count() > 0) || (p_element->non_empty_child_element_count() > 0))
					++count;
			}
		}

		return count;
	}

	void find_children(const std::string & name,std::function<bool(const XMLNode *)> func) const override
	{
		for(auto & p_child : m_children)
			if(p_child && (name.empty() || p_child->value() == name))
				if(func(p_child.get()))
					break;
	}

	void find_elements(const std::string & name,std::function<bool(const XMLElement *)> func) const override
	{
		for(auto & p_child : m_children)
			if(p_child && (p_child->type() == XMLNode::ELEMENT))
				if(name.empty() || (p_child->value() == name))
					if(func(static_cast<const XMLElement *>(p_child.get())))
						break;
	}

	int get_text(std::string & out_string) const
	{
		out_string.clear();
		int count=0;

		for(auto & p_child : m_children)
			if(p_child && (p_child->type() == XMLNode::TEXT))
			{
				out_string.append(static_cast<ade::xml::XMLText *>(p_child.get())->value());
				++count;
			}

		return count;
	}

	int get_element_text(const std::string & element_name,std::string & out_text) const
	{
		int result = 0;

		for(auto & p_child : m_children)
			if(p_child && (p_child->type() == XMLNode::ELEMENT) &&(p_child->value() == element_name))
			{
				result = static_cast<XMLElement *>(p_child.get())->get_text(out_text);
				break;
			}

		return result;
	}

};

//=============================================================================
//
//	COMMENT
//
//=============================================================================

class XMLComment : public XMLNode
{
private:
	std::string		m_comment;

public:
	XMLComment(const std::string & text) :	m_comment(text) {}
	virtual ~XMLComment() = default;

	const std::string & value() const override {return m_comment;}
	type_t				type() const override {return COMMENT;}
	void				find_children(const std::string & /*name*/,std::function<bool(const XMLNode *)>) const override {}
	void				find_elements(const std::string & /*name*/,std::function<bool(const XMLElement *)>) const override {}

	void write(std::ostream & os,int indent) const override
	{
		for(;indent>0;--indent)	os << '\t';
		os << "<!--" << m_comment << "-->\n";
	}
};

//=============================================================================
//
//	DOCUMENT
//
//=============================================================================

class XMLDocument : public XMLElement
{

private:
	std::vector<std::string>			m_errors;
	std::string							m_xml_version;
	std::string							m_encoding;
	std::shared_ptr<IResourceFactory>	m_p_resource_factory;
	DocType								m_doctype;


public:

	XMLDocument()
		: XMLElement("")
	{
	}

	virtual ~XMLDocument() {}

	void set_resource_factory(std::shared_ptr<IResourceFactory>	p_resource_factory)
	{
		m_p_resource_factory = p_resource_factory;
	}

	void clear()
	{
		m_errors.clear();
	}

	void error(const std::string & errstr)
	{
		m_errors.push_back(errstr);
	}

	std::string get_error_string() const 
	{
		std::string errors;

		for(auto & err : m_errors)
		{
			errors.append(err);
			errors.append("\r\n");
		}

		return errors;
	}

	const std::string & get_xml_version()	{return m_xml_version;}
	const std::string & get_encoding()		{return m_encoding;}

	const XMLElement * get_root_element()	{return static_cast<const XMLElement *>(get_first(XMLNode::ELEMENT));}


	bool
	parse_string(	XMLParser::iterator & ib,
					XMLParser::iterator ie )
	{
		while((ib!=ie) && (ib->type==XMLParser::token_t::WHITE_SPACE))
			++ib;

		return ib->type == XMLParser::token_t::STRING;
	}

	bool
	parse_doctype(	XMLParser::iterator & ib,
					XMLParser::iterator ie )
	{
		std::string name;
		std::string public_id;
		std::string system_id;

		//---------------------------------------------------------------------
		//	Get the documents root element name.
		//---------------------------------------------------------------------
		if(!parse_string(ib,ie))
			return true;

		name = ib->text;
		++ib;

		while((ib!=ie) && (ib->type==XMLParser::token_t::WHITE_SPACE))
			++ib;

		//---------------------------------------------------------------------
		//	Get the public and/or system id.
		//---------------------------------------------------------------------
		if(ib->type == XMLParser::token_t::STRING)
		{
			if(ib->text == "PUBLIC")
			{
				++ib;
				if(!parse_string(ib,ie))
					return true;
				public_id = ib->text;
				++ib;
			}
			else if(ib->text != "SYSTEM")
				return true;
			else
				++ib;

			if(!parse_string(ib,ie))
				return true;

			if(ib!=ie)
				system_id = ib++->text;
		}

		//---------------------------------------------------------------------
		//	If the system id is set then load and parse the DTD.
		//---------------------------------------------------------------------
		if(m_p_resource_factory)
		{
			if(!public_id.empty())
			{
				auto data = m_p_resource_factory->load_public(public_id);
				if(data.p_data && data.datasize)
					parse_dtd(data.p_data.get(),data.datasize);
			}

			if(!system_id.empty())
			{
				auto data = m_p_resource_factory->load(system_id);
				if(data.p_data && data.datasize)
					parse_dtd(data.p_data.get(),data.datasize);
			}
		}

		//---------------------------------------------------------------------
		//	Skip to the end of the declaration.
		//---------------------------------------------------------------------
		bool	finished_decl	= false;
		int		open_count		= 0;

		while((ib!=ie) && !finished_decl)
		{
			if(	(ib->type == XMLParser::token_t::START_ELEMENT) ||
				(ib->type == XMLParser::token_t::START_DECLARATION) )
				++open_count;
			else if(ib->type == XMLParser::token_t::END_ELEMENT)
			{
				if(open_count)
					--open_count;
				else
					finished_decl=true;
			}
			++ib;
		}

		if(!finished_decl)
			return true;

		return false;
	}


	void parse_tokens(	XMLParser::iterator & ib,
						XMLParser::iterator ie,
						bool &				err,
						XMLElement *		pparent )
	{
		bool		finished = false;

		if(!pparent)
		{
			err = true;
			return;
		}

		while((ib!=ie) && !finished && !err)
		{
			switch(ib->type)
			{
				//-------------------------------------------------------------
				case	XMLParser::token_t::START_ELEMENT :
				//-------------------------------------------------------------
					++ib;
					if((ie-ib)<2)
					{
						err=true;
						break;
					}

					switch(ib->type)
					{
						//-----------------------------------------------------
						case	XMLParser::token_t::STRING :
						//-----------------------------------------------------
							{
								auto p_element = std::make_unique<XMLElement>(ib->text);
								//std::unique_ptr<XMLElement> p_element(new XMLElement{ib->text});
								if(p_element)
								{
									++ib;
									while(	!err &&
											(ib!=ie) && 
											(ib->type!=XMLParser::token_t::END_ELEMENT) && 
											(ib->type!=XMLParser::token_t::CLOSE_ELEMENT) )
									{
										while((ib!=ie) && (ib->type==XMLParser::token_t::WHITE_SPACE))
											++ib;

										if(ib==ie)
											err = true;
										else if((ib->type!=XMLParser::token_t::END_ELEMENT) && 
												(ib->type!=XMLParser::token_t::CLOSE_ELEMENT))
										{
											if(	((ie-ib)>=3) &&
												(ib->type==XMLParser::token_t::STRING) &&
												((ib+1)->type==XMLParser::token_t::EQUALS) &&
												((ib+2)->type==XMLParser::token_t::STRING) )
											{
												p_element->set_attribute(ib->text,(ib+2)->text);
												ib+=3;
											}
											else 
												err = true;
										}
									}
								
									if(ib==ie)
										err = true;

									//---------------------------------------------
									//	If there was no error then add the element 
									//	to the parent element or set the return 
									//	value.
									//---------------------------------------------
									if(!err)
									{
										if(ib->type == XMLParser::token_t::CLOSE_ELEMENT)
										{
											pparent->push_back(std::move(p_element));
											++ib;
										}
										else
										{
											++ib;
											parse_tokens(ib,ie,err,p_element.get());
											if(!err)
												pparent->push_back(std::move(p_element));
										}
									}
								}
								else
									err = true;
							}
							break;

						//-----------------------------------------------------
						case	XMLParser::token_t::FORWARD_SLASH :
						//-----------------------------------------------------
							++ib;
							if(((ie-ib)<2) || (ib->type!=XMLParser::token_t::STRING) || !pparent)
								err = true;
							else
							{
								if(pparent->value()==ib->text)
								{
									++ib;
									if(ib->type == XMLParser::token_t::END_ELEMENT)
										finished = true;
									else 
										err = true;
								}
								else
									err = true;
							}
							break;

						//-----------------------------------------------------
						default : // ERROR! Unexpected Token
						//-----------------------------------------------------
							err = true; 
							break;
					}
					break;

				//-------------------------------------------------------------
				case	XMLParser::token_t::START_QUESTION_MARK :
				//-------------------------------------------------------------
					//if(pparent)
					//	err = true;
					//else
					{
						++ib;
						if(ib!=ie)
						{
							if(ib->type==XMLParser::token_t::STRING)
							{
								auto type = ib->text;
								std::transform(std::begin(type),std::end(type),std::begin(type),::tolower);

								++ib;

								while(	!err &&
										(ib!=ie) && 
										(ib->type!=XMLParser::token_t::END_QUESTION_MARK) )
								{
									while((ib!=ie) && (ib->type==XMLParser::token_t::WHITE_SPACE))
										++ib;

									if(	((ie-ib)>=3) &&
										(ib->type==XMLParser::token_t::STRING) &&
										((ib+1)->type==XMLParser::token_t::EQUALS) &&
										((ib+2)->type==XMLParser::token_t::STRING) )
									{
										auto id		= ib->text;
										auto & val	= (ib+2)->text;

										std::transform(std::begin(id),std::end(id),std::begin(id),::tolower);

										if(type == "xml")
										{
											if(id == "version")			m_xml_version = val;
											else if(id == "encoding")	m_encoding = val;
											ib += 3;
										}
									}
								}
							}
						}

						while(!err && (ib!=ie) && (ib->type!=XMLParser::token_t::END_QUESTION_MARK))
							++ib;

						if(ib==ie)
							err = true;
						else 
							++ib;
					}
					break;

				//-------------------------------------------------------------
				case	XMLParser::token_t::START_COMMENT :
				//-------------------------------------------------------------
					{
						std::string comment;
						while((ib!=ie) && (ib->type!=XMLParser::token_t::END_COMMENT))
						{
							if(	(ib->type == XMLParser::token_t::STRING) || 
								(ib->type == XMLParser::token_t::TEXT) ||
								(ib->type == XMLParser::token_t::WHITE_SPACE) )
								comment = comment+ib->text;
							++ib;
						}
						if(ib==ie)
							err = true;
						else
						{
							auto p_comment = std::make_unique<XMLComment>(comment);
							if(p_comment)
							{
								pparent->push_back(std::move(p_comment));
								++ib;
							}
							else
								err=true;
						}
					}
					break;

				//-------------------------------------------------------------
				case	XMLParser::token_t::START_DECLARATION :
				//-------------------------------------------------------------
					{
						size_t open_count=0;
						bool finished_decl = false;

						++ib;

						if((ib!=ie) && (ib->type==XMLParser::token_t::STRING) && (ib->text == "DOCTYPE"))
							err = parse_doctype(++ib,ie);
						else
						{
							while((ib!=ie) && !finished_decl)
							{
								if(	(ib->type == XMLParser::token_t::START_ELEMENT) ||
									(ib->type == XMLParser::token_t::START_DECLARATION) )
									++open_count;
								else if(ib->type == XMLParser::token_t::END_ELEMENT)
								{
									if(open_count)
										--open_count;
									else
										finished_decl=true;
								}
								++ib;
							}

							if(!finished_decl)
								err = true;
						}
					}
					break;

				//-------------------------------------------------------------
				case	XMLParser::token_t::TEXT :
				//-------------------------------------------------------------
					{
						auto p_text = std::make_unique<XMLText>(ib->text);
						if(p_text)
						{
							std::unique_ptr<XMLText> p_new_text(new XMLText{ib->text});
							pparent->push_back(std::move(p_new_text));
							++ib;
						} 
						else
							err=true;
					}
					break;

				//-------------------------------------------------------------
				default :
				//-------------------------------------------------------------
					++ib;
					break;
			}
		}
	}

	int parse_dtd_element(	XMLParser::iterator &	ib,
							XMLParser::iterator		ie )
	{
		DocTypeElement element;

		//---------------------------------------------------------------------
		//	Get the element name.
		//---------------------------------------------------------------------
		if(!parse_string(ib,ie))
			return -1;

		std::string element_name(ib++->text);
		if(element_name.empty())
			return -1;

		element.set_name(element_name);

		//---------------------------------------------------------------------
		//	Get the element content.
		//---------------------------------------------------------------------
		std::string content;
		while((ib!=ie) && ((ib->type == XMLParser::token_t::WHITE_SPACE) || (ib->type == XMLParser::token_t::STRING)))
		{
			if(ib->type == XMLParser::token_t::STRING)
				content.append(ib->text);
			++ib;
		}

		//---------------------------------------------------------------------
		//	Parse the element content.
		//---------------------------------------------------------------------
		if(element.parse_content_model(content))
			return -1;

		m_doctype.add_element(std::move(element));

		return 0;
	}


	int parse_dtd_tokens(	XMLParser::iterator &	ib,
							XMLParser::iterator		ie )
	{
		bool	finished	= false;
		bool	err			= false;

		while((ib!=ie) && !finished && !err)
		{
			switch(ib->type)
			{
				//-------------------------------------------------------------
				case	XMLParser::token_t::START_ELEMENT :
				//-------------------------------------------------------------
					err=true;
					break;

				//-------------------------------------------------------------
				case	XMLParser::token_t::START_QUESTION_MARK :
				//-------------------------------------------------------------
					++ib;
					if(ib!=ie)
					{
						if(ib->type==XMLParser::token_t::STRING)
						{
							auto type = ib->text;
							std::transform(std::begin(type),std::end(type),std::begin(type),::tolower);

							++ib;

							while(	!err &&
									(ib!=ie) && 
									(ib->type!=XMLParser::token_t::END_QUESTION_MARK) )
							{
								//while((ib!=ie) && (ib->type==XMLParser::token_t::WHITE_SPACE))
								//	++ib;

								//if(	((ie-ib)>=3) &&
								//	(ib->type==XMLParser::token_t::STRING) &&
								//	((ib+1)->type==XMLParser::token_t::EQUALS) &&
								//	((ib+2)->type==XMLParser::token_t::STRING) )
								//{
								//	if(type == "xml")
								//	{
								//		auto id		= ib->text;
								//		auto & val	= (ib+2)->text;

								//		std::transform(std::begin(id),std::end(id),std::begin(id),::tolower);

								//		if(id == "version")			m_xml_version = val;
								//		else if(id == "encoding")	m_encoding = val;
								//		ib += 3;
								//	}
								//}
								//else
									if(ib!=ie)
										++ib;
							}
						}
					}

					while(!err && (ib!=ie) && (ib->type!=XMLParser::token_t::END_QUESTION_MARK))
						++ib;

					if(ib==ie)
						err = true;
					else 
						++ib;
					break;

				//-------------------------------------------------------------
				case	XMLParser::token_t::START_COMMENT :
				//-------------------------------------------------------------
					{
						std::string comment;
						while((ib!=ie) && (ib->type!=XMLParser::token_t::END_COMMENT))
						{
							if(	(ib->type == XMLParser::token_t::STRING) || 
								(ib->type == XMLParser::token_t::TEXT) ||
								(ib->type == XMLParser::token_t::WHITE_SPACE) )
								comment = comment+ib->text;
							++ib;
						}

						if(ib==ie)
							err = true;
						else
						{
							//auto p_comment = std::make_unique<XMLComment>(comment);
							//if(p_comment)
							//{
							//	pparent->push_back(std::move(p_comment));
								++ib;
							//}
							//else
							//	err=true;
						}
					}
					break;

				//-------------------------------------------------------------
				case	XMLParser::token_t::START_DECLARATION :
				//-------------------------------------------------------------
					{
						size_t open_count=0;
						bool finished_decl = false;

						++ib;

						//-----------------------------------------------------
						//	Type specific declaration parsing.
						//-----------------------------------------------------
						if((ib!=ie) && (ib->type==XMLParser::token_t::STRING) && (ib->text == "ELEMENT"))
							err = !!parse_dtd_element(++ib,ie);

						//-----------------------------------------------------
						//	Flush to the end of the declaration.
						//-----------------------------------------------------
						while(!err && (ib!=ie) && !finished_decl)
						{
							if(	(ib->type == XMLParser::token_t::START_ELEMENT) ||
								(ib->type == XMLParser::token_t::START_DECLARATION) )
								++open_count;
							else if(ib->type == XMLParser::token_t::END_ELEMENT)
							{
								if(open_count)
									--open_count;
								else
									finished_decl=true;
							}
							++ib;
						}

						if(!finished_decl)
							err = true;
					}
					break;

				//-------------------------------------------------------------
				default :
				//-------------------------------------------------------------
					++ib;
					break;
			}
		}

		return err ? -1 : 0;
	}

	int parse(const char * pdata,size_t size)
	{
		clear();
		
		XMLParser parser((char *)pdata,size);

		if(parser.empty())
			return 0;

		XMLParser::iterator ib = parser.begin();
		XMLParser::iterator ie = parser.end();

		//---------------------------------------------------------------------
		//	Strip Leading White Space.
		//---------------------------------------------------------------------
		//while((ib!=ie) && (ib->type==XMLParser::token_t::WHITE_SPACE))
		//	++ib;

		bool err = false;
		parse_tokens(ib,ie,err,this);
		
		return (err ? -1:0);
	}

	int parse_dtd(const char * pdata,size_t size)
	{
		XMLParser parser((char *)pdata,size);

		if(parser.empty())
			return 0;

		auto ib = parser.begin();
		auto ie = parser.end();

		return parse_dtd_tokens(ib,ie);
	}

	bool is_element_an_array(const std::string & parent_name,const std::string & element_name) const
	{
		return m_doctype.is_element_an_array(parent_name,element_name);
	}

};

}} // namnespace xml, ade


//static std::ostream & operator << (std::ostream & os,const ade::xml::XMLNode & node)
//{
//	node.write(os,0);
//	return os;
//}
//
//static std::ostream & operator << (std::ostream & os,const ade::xml::XMLDocument & doc)
//{
//	os << "<?xml version=\"1.0\" encoding=\"utf-8\"?>" << std::endl;
//	doc.write(os,0);
//	return os;
//}
//

#endif // ! defined GUARD_ADE_XML_H
