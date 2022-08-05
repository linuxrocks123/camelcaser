#include <cctype>
#include <fstream>
#include <iostream>
#include <regex>
#include <string>
#include <unordered_set>

using namespace std;

const regex IDENTIFIER("[a-zA-Z_][a-zA-Z0-9_]*",regex_constants::basic);

bool is_inside_quotes(const string& whole_file, int pos)
{
     vector<bool> file_vector;
     bool single_quotes = false;
     bool double_quotes = false;
     bool backslash = false;
     for(int i=0; i<whole_file.size(); i++)
     {
          file_vector.push_back(single_quotes || double_quotes);
          switch(whole_file[i])
          {
          case '\\': backslash = !backslash;
               break;
          case '\'': if(!backslash && !double_quotes)
                    single_quotes = !single_quotes;
               break;
          case '\"': if(!backslash && !single_quotes)
                    double_quotes = !double_quotes;
               break;
          }
          if(whole_file[i]!='\\')
               backslash = false;

          //Comments: C++-style
          if(!single_quotes && !double_quotes && i && whole_file[i]=='/' && whole_file[i-1]=='/')
          {
               while(whole_file[++i]!='\n')
                    file_vector.push_back(true);
               file_vector.push_back(false);
          }

          /*Comments: C-style
           */
          if(!single_quotes && !double_quotes && i && whole_file[i]=='*' && whole_file[i-1]=='/')
          {
               ++i; file_vector.push_back(true);
               while(whole_file[i]!='*' || whole_file[i+1]!='/')
               {
                    ++i;
                    file_vector.push_back(true);
               }
               file_vector.push_back(false);
          }
                    
     }
     return file_vector[pos];
}

bool already_camelcase(const string& str)
{
     bool to_return = false;
     for(char c : str)
          if(c=='_')
               return false;
          else if(c >= 'A' && c <= 'Z')
               to_return = true;
     return to_return;
}

void token_replace(string& whole_file, const string& old_str, const string& new_str)
{
     if(old_str==new_str)
          return;
     
     auto words_end = sregex_iterator();
     for(auto words_begin = sregex_iterator(whole_file.begin(), whole_file.end(), IDENTIFIER); words_begin!=words_end; ++words_begin)
          if(words_begin->str()==old_str && !is_inside_quotes(whole_file,(*words_begin)[0].first - whole_file.begin()))
          {
               whole_file.replace((*words_begin)[0].first,(*words_begin)[0].second,new_str);
               return token_replace(whole_file,old_str,new_str);
          }
}

string underscore_to_camelcase(string underscore, bool var_type)
{
     for(int i=0; i<underscore.size(); i++)
          if(underscore[i]=='_')
          {
               underscore.erase(i,1);
               underscore[i] = toupper(underscore[i]);
          }

     if(var_type)
          underscore[0] = toupper(underscore[0]);
     return underscore;
}

string line_for(const string& whole_file, int pos)
{
     int line_begin = 0;
     int line_end = whole_file.size();
     for(int i=pos; i>=0; i--)
          if(whole_file[i]=='\n')
          {
               line_begin = i+1;
               break;
          }
     for(int i=pos; i<whole_file.size(); i++)
          if(whole_file[i]=='\n')
          {
               line_end = i;
               break;
          }
     return whole_file.substr(line_begin,line_end - line_begin);
}

int main(int argc, char** argv)
{
     unordered_set<string> ignored_identifiers;
     ifstream ignored_dict{"/home/psimmons/.cppiddict"};
     string next_ignored_id;
     while(getline(ignored_dict,next_ignored_id))
          ignored_identifiers.insert(next_ignored_id);

     ifstream fin{argv[1]};
     string whole_file;
     getline(fin,whole_file,'\0');

     const regex CONSTANT_IDENTIFIER("[A-Z_]*",regex_constants::basic);
     auto tokens_end = sregex_iterator();
loop_start:
     for(auto token = sregex_iterator(whole_file.begin(),whole_file.end(),IDENTIFIER); token!=tokens_end; ++token)
          if(!regex_match(token->str(),CONSTANT_IDENTIFIER) && !ignored_identifiers.count(token->str()) && !is_inside_quotes(whole_file,(*token)[0].first - whole_file.begin()) && !already_camelcase(token->str()))
          {
               char c = '\0';
               while(c!='v' && c!='f' && c!='i' && c!='s')
               {
                    cout << line_for(whole_file,(*token)[0].first - whole_file.begin()) << endl << token->str() << " (v,f,i,s) ";
                    cin >> c;
               }
               if(c=='v' || c=='f')
               {
                    string new_name = underscore_to_camelcase(token->str(),c=='v');
                    token_replace(whole_file,token->str(),new_name);
                    if(!already_camelcase(new_name))
                         ignored_identifiers.insert(new_name);
                    goto loop_start;
               }
               else
               {
                    if(c=='i')
                    {
                         ofstream fout{"/home/psimmons/.cppiddict",ios_base::app};
                         fout << token->str() << endl;
                    }
                    ignored_identifiers.insert(token->str());
               }
          }

     ofstream whole_out{argv[2]};
     whole_out << whole_file;

     return 0;
}
