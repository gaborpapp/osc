//
//  Argument.h
//  Test
//
//  Created by ryan bartley on 8/28/15.
//
//

namespace osc {
	
enum class ArgType {
	INTEGER,
	FLOAT,
	STRING,
	BLOB,
	MIDI,
	NONE
};

class Argument {
public:
	template<typename T>
	Argument( T data );
	
	Argument( const Argument &arg );
	Argument( Argument &&arg );
	Argument& operator=( const Argument &arg );
	Argument& operator=( Argument &&arg );
	
	~Argument() = default;
	
	ArgType getArgType() const { return mType; }
	uint32_t getArgSize() const { return mSize; }
	
	float		getArgAsFloat() const;
	int32_t		getArgAsInt() const;
	std::string getArgAsString() const;
	
private:
	ArgType		mType;
	uint32_t	mSize;
	std::unique_ptr<uint8_t[]> mData;
	
	friend class Message;
};
	
template<>
Argument::Argument( int32_t data )
: mType( ArgType::INTEGER ), mSize( 4 ), mData( new uint8_t[mSize] )
{
	memcpy( mData.get(), &data, mSize );
}

template<>
Argument::Argument( float data )
: mType( ArgType::FLOAT ), mSize( 4 ), mData( new uint8_t[mSize] )
{
	memcpy( mData.get(), &data, mSize );
}

template<>
Argument::Argument( std::string data )
: mType( ArgType::STRING ), mSize( data.size() ), mData( new uint8_t[mSize] )
{
	memcpy( mData.get(), data.data(), data.size() );
}
	
// Blob impl
//	template<typename T>
//	Argument::Argument( T data )
//	{
//		
//	}

}