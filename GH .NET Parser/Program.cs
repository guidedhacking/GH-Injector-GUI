/*
 * Author:       Broihon
 * Copyright:    Guided Hacking™ © 2012-2023 Guided Hacking LLC
*/

using System.Reflection;
using System.Runtime.InteropServices;
using System.IO.Pipes;

namespace GH_DotNet_Parser
{
    public class Tree
    {
        public List<Tree> nodes;
        public String Data;

        public Tree(string data)
        {
            Data = data;
            nodes = new List<Tree>();
        }

        public Tree? SearchNode(String value)
        {
            foreach (var node in nodes)
            {
                if (node.Data == value)
                {
                    return node;
                }
            }

            return null;
        }

        public Tree AddNode(String value)
        {
            nodes.Add(new(value));

            return nodes.Last();
        }

        public void SendToPipe(String pipe_name)
        {
            String output = "";

            foreach (var namespace_node in nodes)
            {
                var current_namespace = namespace_node.Data;
                foreach (var classname_node in namespace_node.nodes)
                {
                    var current_classname = classname_node.Data;
                    foreach (var methodname_node in classname_node.nodes)
                    {
                        var current_methodname = methodname_node.Data;

                        output += current_namespace + ";" + current_classname + ";" + current_methodname + "!";
                    }
                }
            }

            Console.WriteLine("Opening pipe to host process");

            var client = new NamedPipeClientStream(".", pipe_name);
            if (client == null)
            {
                Console.WriteLine("Failed to connect to pipe: " + pipe_name);

                return;
            }

            client.Connect();

            var writer = new StreamWriter(client);
            if (writer == null)
            {
                Console.WriteLine("Failed to create write for pipe client");

                client.Close();

                return;
            }

            writer.WriteLine(output);
            writer.Close();
            client.Close();

            Console.WriteLine("Assembly data sent to pipe successfully");
        }
    }

    public class PARSER
    {
        static void Main(String[] args)
        {
            if (args.Length != 2)
            {
                Console.WriteLine("Invalid parameter count");

                return;
            }

            if (!args[0].EndsWith(".dll", StringComparison.OrdinalIgnoreCase))
            {
                Console.WriteLine("Invalid path provided: " + args[0]);

                return;
            }

            var dll_path = args[0];
            var pipe_name = args[1];

            if (!File.Exists(dll_path))
            {
                Console.WriteLine("File doesn't exist");

                return;
            }

            string[] runtimeAssemblies = Directory.GetFiles(RuntimeEnvironment.GetRuntimeDirectory(), "*.dll");
            var paths = new List<string>(runtimeAssemblies);
            var resolver = new PathAssemblyResolver(paths);

            var mlc = new MetadataLoadContext(resolver);
            if (mlc == null)
            {
                Console.WriteLine("Failed to create MetadataLoadContext");

                return;
            }

            Tree root = new("");

            Console.WriteLine("Parsing assembly: " + dll_path);

            Assembly assembly = mlc.LoadFromAssemblyPath(dll_path);
            foreach (System.Type attr in assembly.GetTypes())
            {
                MemberInfo[] members;
                try
                {
                    members = attr.GetMembers();
                }
                catch (FileNotFoundException)
                {
                    continue;
                }

                foreach (MemberInfo member in members)
                {
                    if (attr.Namespace == null)
                    {
                        continue;
                    }

                    if (member.MemberType != MemberTypes.Method)
                    {
                        continue;
                    }

                    var method = (MethodInfo)member;
                    if (!method.IsPublic || method.ReturnType.ToString() != typeof(int).ToString())
                    {
                        continue;
                    }

                    var parameters = method.GetParameters();
                    if (parameters.Length != 1)
                    {
                        continue;
                    }

                    if (parameters[0].ParameterType.ToString() != typeof(string).ToString())
                    {
                        continue;
                    }

                    Tree? dag_namespace = root.SearchNode(attr.Namespace.ToString());
                    dag_namespace ??= root.AddNode(attr.Namespace.ToString());

                    Tree? dag_class = dag_namespace.SearchNode(attr.Name.ToString());
                    dag_class ??= dag_namespace.AddNode(attr.Name.ToString());

                    if (dag_class.SearchNode(method.Name.ToString()) == null)
                    {
                        dag_class.AddNode(method.Name.ToString());
                    }
                }
            }

            Console.WriteLine("Finished parsing assembly");

            if (!root.nodes.Any())
            {
                Console.WriteLine("No valid data found in the assembly");

                return;
            }

            root.SendToPipe(pipe_name);
        }
    }
}