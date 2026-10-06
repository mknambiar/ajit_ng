#include <stdint.h>
typedef uint16_t half;

static inline int16_t lane_as_i16(uint16_t h)
{
	return (int16_t) h;
}

static inline uint16_t i16_as_lane(int32_t v)
{
	return (uint16_t) ((int16_t) v);
}

static inline uint64_t map4_lanes_u16(uint64_t x, uint64_t y, uint8_t op)
{
	uint64_t out = 0;
	int i;
	for (i = 0; i < 4; i++) {
		uint16_t lx = (uint16_t) ((x >> (16 * i)) & 0xffffu);
		uint16_t ly = (uint16_t) ((y >> (16 * i)) & 0xffffu);
		int32_t ax = lane_as_i16(lx);
		int32_t ay = lane_as_i16(ly);
		int32_t r = 0;
		if (op == 0) r = ax + ay;
		else if (op == 1) r = ax - ay;
		else r = ax * ay;
		out |= ((uint64_t) i16_as_lane(r) << (16 * i));
	}
	return out;
}

half addHalf(half x, half y)
{
	return i16_as_lane((int32_t) lane_as_i16(x) + (int32_t) lane_as_i16(y));
}

half subHalf(half x, half y)
{
	return i16_as_lane((int32_t) lane_as_i16(x) - (int32_t) lane_as_i16(y));
}

half mulHalf(half x, half y)
{
	return i16_as_lane((int32_t) lane_as_i16(x) * (int32_t) lane_as_i16(y));
}

half floatToHalf(float x)
{
	return i16_as_lane((int32_t) x);
}

float halfToFloat(half x)
{
	return (float) lane_as_i16(x);
}

int halfToInt(half x)
{
	return (int) lane_as_i16(x);
}

half intToHalf(int x)
{
	return i16_as_lane((int32_t) x);
}

uint64_t vectorHalfPrecisionOpInU64Form(uint64_t x, uint64_t y, uint8_t op)
{
	return map4_lanes_u16(x, y, op);
}

uint64_t vectorHalfPrecisionAddInU64Form(uint64_t x, uint64_t y)
{
	return map4_lanes_u16(x, y, 0);
}

uint64_t vectorHalfPrecisionSubInU64Form(uint64_t x, uint64_t y)
{
	return map4_lanes_u16(x, y, 1);
}

uint64_t vectorHalfPrecisionMulInU64Form(uint64_t x, uint64_t y)
{
	return map4_lanes_u16(x, y, 2);
}

uint64_t halfPrecisionToI16VectorConvert(uint64_t cvt_op)
{
	return cvt_op;
}

uint64_t i16ToHalfPrecisionVectorConvert(uint64_t cvt_op)
{
	return cvt_op;
}

uint32_t vectorHalfAddReduce(uint64_t cvt_op)
{
	int32_t sum = 0;
	int i;
	for (i = 0; i < 4; i++) {
		uint16_t lane = (uint16_t) ((cvt_op >> (16 * i)) & 0xffffu);
		sum += lane_as_i16(lane);
	}
	return (uint32_t) sum;
}

uint32_t halfToFloatInU32Form(half operand)
{
	union {
		float f;
		uint32_t u;
	} cvt;
	cvt.f = halfToFloat(operand);
	return cvt.u;
}

half floatToHalfInU32Form(uint32_t operand)
{
	union {
		float f;
		uint32_t u;
	} cvt;
	cvt.u = operand;
	return floatToHalf(cvt.f);
}
