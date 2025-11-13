#ifndef MIMETYPERESOLVER_HPP
#define MIMETYPERESOLVER_HPP

#include <string>
#include <vector>

class MimeTypeResolver
{
public:
	struct MagicRule
	{
		size_t offset;
		std::string magic;
		std::string mimeType;
	};

	static std::vector<MagicRule> &getMagicRules()
	{
		static std::vector<MagicRule> rules;
		if (rules.empty())
		{
			MagicRule rule1;
			rule1.offset = 0;
			rule1.magic = "\\x89PNG\\r\\n\\x1a\\n";
			rule1.mimeType = "image/png";
			rules.push_back(rule1);

			MagicRule rule2;
			rule2.offset = 0;
			rule2.magic = "%PDF-";
			rule2.mimeType = "application/pdf";
			rules.push_back(rule2);

			MagicRule rule3;
			rule3.offset = 0;
			rule3.magic = "PK\\x03\\x04";
			rule3.mimeType = "application/zip";
			rules.push_back(rule3);

			MagicRule rule4;
			rule4.offset = 0;
			rule4.magic = "\\xFF\\xD8\\xFF";
			rule4.mimeType = "image/jpeg";
			rules.push_back(rule4);

			MagicRule rule5;
			rule5.offset = 0;
			rule5.magic = "GIF87a";
			rule5.mimeType = "image/gif";
			rules.push_back(rule5);

			MagicRule rule6;
			rule6.offset = 0;
			rule6.magic = "GIF89a";
			rule6.mimeType = "image/gif";
			rules.push_back(rule6);
		}
		return rules;
	}

	static bool &getInitialized()
	{
		static bool instance = false;
		return instance;
	}

	static std::string resolveMimeType(const std::string &filePath);
	static std::string resolveMimeTypeByExtension(const std::string &filePath);
	static std::string resolveMimeTypeByMagic(const std::string &filePath);
	static void initialize();
	static void cleanup();
};

#endif /* MIMETYPERESOLVER_HPP */