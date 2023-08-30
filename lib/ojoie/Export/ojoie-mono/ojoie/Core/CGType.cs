using System.Runtime.InteropServices;

namespace AN;

[StructLayout(LayoutKind.Sequential)]
public struct Size
{
    public uint Width;
    public uint Height;

    public Size(uint width, uint height)
    {
        Width = width;
        Height = height;
    }
}

[StructLayout(LayoutKind.Sequential)]
public struct Point
{
    public int X;
    public int Y;

    public Point(int x, int y)
    {
        X = x;
        Y = y;
    }
}

[StructLayout(LayoutKind.Sequential)]
public struct Rect
{
    public Point Origin;
    public Size Size;

    public Rect(int x, int y, uint width, uint height)
    {
        Size = new Size(width, height);
        Origin = new Point(x, y);
    }

    public uint Width
    {
        get => Size.Width;
        set => Size.Width = value;
    }

    public uint Height
    {
        get => Size.Height;
        set => Size.Height = value;
    }

    public int X
    {
        get => Origin.X;
        set => Origin.X = value;
    }

    public int Y
    {
        get => Origin.Y;
        set => Origin.Y = value;
    }
}