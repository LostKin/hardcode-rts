varying highp vec2 texture_coords;

uniform sampler2D texture;

void main ()
{
    gl_FragColor = texture2D(texture, texture_coords);
}
