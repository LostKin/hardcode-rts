#version 400

in highp vec2 texture_coords;

uniform float texture_mode_rect;
uniform sampler2D texture_2d;
uniform sampler2DRect texture_2d_rect;

void main ()
{
    if (texture_mode_rect != 0.0) {
        ivec2 ts = textureSize (texture_2d_rect);
        gl_FragColor = texelFetch (texture_2d_rect, ivec2 (texture_coords)%ts);
    } else {
        gl_FragColor = texture2D (texture_2d, texture_coords);
    }
}
