attribute highp vec2 position_attr;
attribute lowp vec4 color_attr;
attribute highp vec2 texture_coords_attr;

varying lowp vec4 color;
varying highp vec2 texture_coords;

uniform highp mat4 ortho_matrix;

void main ()
{
    color = color_attr;
    texture_coords = texture_coords_attr;
    gl_Position = ortho_matrix*vec4 (position_attr, 0.0, 1.0);
}
