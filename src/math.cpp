internal inline f32
min(f32 a, f32 b) {
    if(a < b) {
        return a;
    } else {
        return b;
    }
}

internal inline f32
max(f32 a, f32 b) {
    if(a > b) {
        return a;
    } else {
        return b;
    }
}

internal inline f32
clamp(f32 value, f32 _min, f32 _max) {
    f32 result = min(max(value, _min), _max);
    return value;
}

// Normalize or zero
internal inline v2
noz(v2 input) {
    v2 result = input.x || input.y ? glm::normalize(input) : v2(0, 0);
    return result;
}
