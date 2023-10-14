//! \file       dedsm.cs
//! \date       2019 Apr 19
//! \brief      Decrypt Unity DSM script.
//

using System;
using System.IO;
using System.Runtime.InteropServices;
using System.Security.Cryptography;
using System.Text;

class DsmDecryptor
{
    private static void GenerateKeyFromPassword (string password, int keySize, out byte[] key, int blockSize, out byte[] iv)
    {
        var bytes = Encoding.UTF8.GetBytes ("saltは必ず8バイト以上");
        var rfc2898DeriveBytes = new Rfc2898DeriveBytes (password, bytes);
        rfc2898DeriveBytes.IterationCount = 1000;
        key = rfc2898DeriveBytes.GetBytes (keySize / 8);
        iv = rfc2898DeriveBytes.GetBytes (blockSize / 8);
    }

    static string DecryptString (string sourceString, string password)
    {
        var rijndaelManaged = new RijndaelManaged();
        byte[] key, iv;
        GenerateKeyFromPassword (password, rijndaelManaged.KeySize, out key, rijndaelManaged.BlockSize, out iv);
        rijndaelManaged.Key = key;
        rijndaelManaged.IV = iv;
        var array = Convert.FromBase64String (sourceString);
        using (var cryptoTransform = rijndaelManaged.CreateDecryptor())
        {
            var bytes = cryptoTransform.TransformFinalBlock (array, 0, array.Length);
            return Encoding.UTF8.GetString (bytes);
        }
    }

    public static void Main (string[] args)
    {
        if (args.Length < 1)
            return;
    	using (var reader = new StreamReader (args[0]))
        {
            var sourceString = reader.ReadToEnd();
            var text = DecryptString (sourceString, "pass");
            Console.OutputEncoding = Encoding.UTF8;
            Console.Write (text);
        }
    }
}
