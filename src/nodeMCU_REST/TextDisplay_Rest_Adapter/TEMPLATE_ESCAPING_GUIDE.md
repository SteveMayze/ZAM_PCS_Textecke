# Template Placeholder Escaping Guide

## Configuration

The `TEMPLATE_PLACEHOLDER` is set to `~` (tilde) via build flags in `platformio.ini`:
```ini
-D TEMPLATE_PLACEHOLDER='~'
```

This persists across clean builds and doesn't require manual library modifications.

## Usage in Templates

### Variable Substitution
Use single placeholder pairs for template variables:
```html
<p>Welcome ~USERNAME~!</p>
<p>Your balance is ~BALANCE~</p>
```

### Escaping Literal Tilde Characters
**The ESPAsyncWebServer library supports double-character escaping.**

To display a literal `~` character in your HTML/text, use **two consecutive tildes**:

| Input | Output |
|-------|--------|
| `~~` | `~` |
| `~~~~` | `~~` |
| `Text with ~~ tilde` | `Text with ~ tilde` |
| `Price: ~~50` | `Price: ~50` |

### User Input Handling

When users enter text that might contain `~` characters:

1. **For display in templates**: The user's input should have single `~` converted to `~~`
2. **For database storage**: Store the original text as-is
3. **Before template rendering**: Escape by replacing `~` → `~~`

#### Example Escaping Function

```cpp
String escapeTemplateChars(const String& input) {
  String output;
  output.reserve(input.length() * 2); // Pre-allocate
  
  for (unsigned int i = 0; i < input.length(); i++) {
    if (input[i] == TEMPLATE_PLACEHOLDER) {
      output += TEMPLATE_PLACEHOLDER; // Add twice
    }
    output += input[i];
  }
  
  return output;
}
```

#### Usage Example

```cpp
// User submits text: "Save ~50% off!"
String userText = request->getParam("message")->value();

// Store in database as-is
database.save(userText); // Stores: "Save ~50% off!"

// When displaying in template
String escapedText = escapeTemplateChars(userText); // "Save ~~50% off!"
response->setContent(escapedText); // Displays: "Save ~50% off!"
```

## Why Tilde (~)?

The tilde character was chosen because:
- ✅ Doesn't conflict with CSS (unlike `%` which breaks CSS percentages)
- ✅ Doesn't conflict with common user text (unlike `$` for currency)
- ✅ Not used in URLs, HTML tags, or JavaScript
- ✅ Easy to type and visually distinct
- ✅ Can be escaped when needed using `~~`

## Alternative Characters

If `~` doesn't work for your use case, you can change it in `platformio.ini`. Good alternatives:
- `'§'` (section sign)
- `'¤'` (currency sign)
- `'¦'` (broken bar)

## Important Notes

1. **Escaping is automatic** for paired placeholders (e.g., `~~` → `~`)
2. **Single unmatched** placeholders are rendered as-is if no closing placeholder is found
3. **Always escape user input** before using it in templates
4. The build flag approach ensures this persists across clean builds
